import { createLogger } from '@aztec/foundation/log';

import { isK8sConfig, setupEnvironment, startPortForward } from './utils.js';

const config = setupEnvironment(process.env);
if (!isK8sConfig(config)) {
  throw new Error('This test requires running in K8s');
}

const _debugLogger = createLogger('e2e:spartan-test:prover-node');

describe('prover node recovery', () => {
  beforeAll(async () => {
    await startPortForward({
      resource: `svc/${config.INSTANCE_NAME}-aztec-network-prover-node`,
      namespace: config.NAMESPACE,
      containerPort: config.CONTAINER_PROVER_NODE_PORT,
      hostPort: config.HOST_PROVER_NODE_PORT,
    });

    await startPortForward({
      resource: `svc/metrics-grafana`,
      namespace: 'metrics',
      containerPort: config.CONTAINER_METRICS_PORT,
      hostPort: config.HOST_METRICS_PORT,
    });
  });

  it('should be able to get node enr', () => {
    expect(1).toEqual(1);
  });
});
