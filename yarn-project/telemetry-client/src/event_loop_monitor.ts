import { promiseWithResolvers } from '@aztec/foundation/promise';
import { Timer } from '@aztec/foundation/timer';

import { AsyncHook, HookCallbacks, createHook } from 'node:async_hooks';
import { type EventLoopUtilization, performance } from 'node:perf_hooks';

import { EVENT_LOOP_LAG, EVENT_LOOP_UTILIZATION } from './metrics.js';
import { type BatchObservableResult, type Meter, type ObservableGauge, Tracer, ValueType } from './telemetry.js';

type AsyncOpMeta = {
  type: string;
  startAtNs: bigint;
  startAtEpoch: number;
  stack?: string;
};

/**
 * Detector for custom Aztec attributes
 */
export class EventLoopMonitor {
  private eventLoopLag: ObservableGauge;
  private eventLoopUilization: ObservableGauge;
  private started = false;

  private lastELU: EventLoopUtilization | undefined;

  private runningAsyncOps = new Map<number, AsyncOpMeta>();

  private hook: AsyncHook;

  constructor(
    private meter: Meter,
    private tracer: Tracer,
    private loopBlockedThresholdNS: bigint = /* 50ms */ 5n * 10n ** 7n,
  ) {
    this.eventLoopLag = meter.createObservableGauge(EVENT_LOOP_LAG, {
      unit: 'us',
      valueType: ValueType.INT,
      description: 'Latency to execute a macro task',
    });
    this.eventLoopUilization = meter.createObservableGauge(EVENT_LOOP_UTILIZATION, {
      valueType: ValueType.DOUBLE,
      description: 'How busy is the event loop',
    });

    this.hook = createHook({
      before: this.before,
      destroy: this.destroy,
      init: this.init,
      after: this.after,
    });
  }

  start(): void {
    if (this.started) {
      return;
    }

    this.lastELU = performance.eventLoopUtilization();
    this.meter.addBatchObservableCallback(this.measureLag, [this.eventLoopUilization, this.eventLoopLag]);
    this.hook.enable();
  }

  stop(): void {
    if (!this.started) {
      return;
    }
    this.hook.disable();
    this.meter.removeBatchObservableCallback(this.measureLag, [this.eventLoopUilization, this.eventLoopLag]);
  }

  private measureLag = async (obs: BatchObservableResult): Promise<void> => {
    const newELU = performance.eventLoopUtilization();
    const delta = performance.eventLoopUtilization(newELU, this.lastELU);
    this.lastELU = newELU;

    const timer = new Timer();
    const { promise, resolve } = promiseWithResolvers<number>();
    // how long does it take to schedule the next macro task?
    // if this number spikes then we're (1) either blocking the event loop with long running sync code
    // or (2) spamming the event loop with micro tasks
    setImmediate(() => {
      resolve(timer.us());
    });

    const lag = await promise;
    obs.observe(this.eventLoopLag, Math.floor(lag));
    obs.observe(this.eventLoopUilization, delta.utilization);
  };

  private init: HookCallbacks['init'] = (asyncId, type) => {
    this.runningAsyncOps.set(asyncId, { type, startAtNs: 0n, startAtEpoch: 0, stack: new Error().stack });
  };

  private destroy: HookCallbacks['destroy'] = asyncId => {
    this.runningAsyncOps.delete(asyncId);
  };

  private before: HookCallbacks['before'] = asyncId => {
    const meta = this.runningAsyncOps.get(asyncId);
    if (meta) {
      meta.startAtNs = process.hrtime.bigint();
      meta.startAtEpoch = Date.now();
    }
  };

  private after: HookCallbacks['after'] = asyncId => {
    const meta = this.runningAsyncOps.get(asyncId);
    if (!meta) {
      return;
    }

    const now = process.hrtime.bigint();
    const duration = now - meta.startAtNs;
    if (duration > this.loopBlockedThresholdNS) {
      const span = this.tracer.startSpan('EventLoopBlocked', {
        startTime: meta.startAtEpoch, // can't use startAtNs because its origin is unknown
        attributes: {
          stack: meta.stack,
        } as any,
      });
      span.end();
    }

    this.runningAsyncOps.delete(asyncId);
  };
}
