import { AztecNode, createAztecNodeClient, createCompatibleClient, retryUntil, sleep } from '@aztec/aztec.js';
import { RollupCheatCodes } from '@aztec/aztec.js/utils';
import { EthCheatCodesWithState } from '@aztec/ethereum/test';
import { createLogger } from '@aztec/foundation/log';
import { type ProverNodeApi, createProverNodeRpcClient } from '@aztec/prover-node';

import { type ChildProcess, execSync } from 'node:child_process';

import { type AlertConfig } from '../quality_of_service/alert_checker.js';
import { applyProverKill, isK8sConfig, setupEnvironment, startPortForward } from './utils.js';

const config = setupEnvironment(process.env);
if (!isK8sConfig(config)) {
  throw new Error('This test requires running in K8s');
}

const logger = createLogger('e2e:spartan-test:prover-node');

const _qosAlerts: AlertConfig[] = [
  {
    // Checks that we are not proving from scratch every time
    alert: 'ProverBrokerCachedRate',
    expr: 'rate(aztec_proving_queue_cached_jobs[1m]) > 0.5',
    labels: { severity: 'error' },
    for: '10m',
    annotations: {},
  },
];

describe('prover node recovery', () => {
  let proverNode: ProverNodeApi;
  let node: AztecNode;
  let pf: ChildProcess;
  beforeAll(async () => {
    pf = await startPortForward({
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

    proverNode = createProverNodeRpcClient(`http://127.0.0.1:${config.HOST_PROVER_NODE_PORT}`, 'proverNode');
    node = createAztecNodeClient(`http://127.0.0.1:${config.HOST_PROVER_NODE_PORT}`);
  });

  it('should start proving', async () => {
    // const nodeInfo = await node.getNodeInfo();
    // const ethCheatCodes = new EthCheatCodesWithState(`http://127.0.0.1:${config.HOST_ETHEREUM_PORT}`);
    // const rollupCheatCodes = new RollupCheatCodes(ethCheatCodes, nodeInfo.l1ContractAddresses);

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

    pf.kill();
    await sleep(5000);
    logger.info('About to kill the prover node pod');
    execSync(`kubectl delete pod -n ${config.NAMESPACE} -l app=prover-node`, {
      stdio: 'inherit',
    });
    logger.info('About prover node killed');

    execSync(`kubectl get pods -n ${config.NAMESPACE} -l app=prover-node`, {
      stdio: 'inherit',
    });

    execSync(`kubectl wait pod -l app==prover-node --for=condition=Ready -n ${config.NAMESPACE} --timeout=10m`, {
      stdio: 'inherit',
    });

    pf = await startPortForward({
      resource: `svc/${config.INSTANCE_NAME}-aztec-network-prover-node`,
      namespace: config.NAMESPACE,
      containerPort: config.CONTAINER_PROVER_NODE_PORT,
      // hostPort: 42069,
      hostPort: config.HOST_PROVER_NODE_PORT,
    });

    // proverNode = createProverNodeRpcClient(`http://127.0.0.1:${42069}`, 'proverNode');

    await sleep(5000);

    console.log(await fetch(`http://127.0.0.1:${config.HOST_PROVER_NODE_PORT}`));
    // console.log(await fetch(`http://127.0.0.1:42069`));

    const newActiveJobs = await retryUntil(async () => {
      try {
        const status = await proverNode.getJobs();
        return status.length > 0 && status[0].status === 'awaiting-prover';
      } catch (err) {
        // logger.warn('Error getting jobs: ', err);
      }
    });

    logger.info('New active jobs: ', newActiveJobs);
  }, 3_600_000);
});
