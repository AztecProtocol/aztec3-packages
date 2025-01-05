import { retryUntil, sleep } from '@aztec/aztec.js';
import { createLogger } from '@aztec/foundation/log';
import { type ProverNodeApi, createProverNodeRpcClient } from '@aztec/prover-node';

import { type ChildProcess, execSync } from 'node:child_process';

import { type AlertConfig } from '../quality_of_service/alert_checker.js';
import { isK8sConfig, runAlertCheck, setupEnvironment, startPortForward } from './utils.js';

const config = setupEnvironment(process.env);
if (!isK8sConfig(config)) {
  throw new Error('This test requires running in K8s');
}

const logger = createLogger('e2e:spartan-test:prover-node');

const PROOF_TYPE = 'TUBE_PROOF';
const _qosAlerts: AlertConfig[] = [
  {
    // Checks that we are not proving from scratch every time
    alert: 'ProverBrokerCachedRate',
    expr: `rate(aztec_proving_queue_cached_jobs{aztec_proving_job_type="${PROOF_TYPE}"}[5m]) > 0`,
    labels: { severity: 'error' },
    for: '1m',
    annotations: {},
  },
];

describe('prover node recovery', () => {
  let proverNode: ProverNodeApi;
  let pf: ChildProcess;
  let pfs: ChildProcess[];
  beforeAll(async () => {
    pfs = [];
    pf = await startPortForward({
      resource: `svc/${config.INSTANCE_NAME}-aztec-network-prover-node`,
      namespace: config.NAMESPACE,
      containerPort: config.CONTAINER_PROVER_NODE_PORT,
      hostPort: config.HOST_PROVER_NODE_PORT,
    });

    pfs.push(pf);

    pfs.push(
      await startPortForward({
        resource: `svc/metrics-grafana`,
        namespace: 'metrics',
        containerPort: config.CONTAINER_METRICS_PORT,
        hostPort: config.HOST_METRICS_PORT,
      }),
    );

    proverNode = createProverNodeRpcClient(`http://127.0.0.1:${config.HOST_PROVER_NODE_PORT}`, 'proverNode');
  });

  afterAll(() => {
    for (const pf of pfs) {
      pf.kill();
    }
  });

  it('should start proving', async () => {
    const activeJobs = await retryUntil(async () => {
      const status = await proverNode.getJobs();
      return status.length > 0 && status[0].status === 'awaiting-prover';
    });

    logger.info('Active jobs: ', activeJobs);

    // await applyProverKill({
    //   namespace: config.NAMESPACE,
    //   spartanDir: config.SPARTAN_DIR,
    //   logger,
    // });

    // execSync(`lsof -i :${config.HOST_PROVER_NODE_PORT}`, {
    //   stdio: 'inherit',
    // });

    // pf.kill();
    // await sleep(5000);

    // logger.info('AFter pf kill');
    // execSync(`lsof -i :${config.HOST_PROVER_NODE_PORT} || true`, {
    //   stdio: 'inherit',
    // });
    logger.info('About to kill the prover node pod');
    execSync(`kubectl delete pod -n ${config.NAMESPACE} -l app=prover-node`, {
      stdio: 'inherit',
    });
    logger.info('Prover node killed');

    execSync(`kubectl get pods -n ${config.NAMESPACE} -l app=prover-node`, {
      stdio: 'inherit',
    });

    execSync(`kubectl wait pod -l app==prover-node --for=condition=Ready -n ${config.NAMESPACE} --timeout=10m`, {
      stdio: 'inherit',
    });

    // pf = await startPortForward({
    //   resource: `svc/${config.INSTANCE_NAME}-aztec-network-prover-node`,
    //   namespace: config.NAMESPACE,
    //   containerPort: config.CONTAINER_PROVER_NODE_PORT,
    //   // hostPort: 42069,
    //   hostPort: config.HOST_PROVER_NODE_PORT,
    // });

    // logger.info('After pf start');
    // execSync(`lsof -i :${config.HOST_PROVER_NODE_PORT} || true`, {
    //   stdio: 'inherit',
    // });

    // proverNode = createProverNodeRpcClient(`http://127.0.0.1:${42069}`, 'proverNode');

    await sleep(5000);

    await expect(runAlertCheck(config, _qosAlerts, logger)).rejects.toBeUndefined();

    // console.log(await fetch(`http://127.0.0.1:${config.HOST_PROVER_NODE_PORT}`));
    // console.log(await fetch(`http://127.0.0.1:42069`));

    // const newActiveJobs = await retryUntil(async () => {
    //   try {
    //     const status = await proverNode.getJobs();
    //     return status.length > 0 && status[0].status === 'awaiting-prover';
    //   } catch (err) {
    //     // logger.warn('Error getting jobs: ', err);
    //   }
    // });

    // logger.info('New active jobs: ', newActiveJobs);
  }, 3_600_000);
});
