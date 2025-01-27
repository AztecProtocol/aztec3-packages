import { type EpochProverManager } from '@aztec/circuit-types';

import { type L1TxPublisher } from '../l1-tx-publisher.js';
import { ProverNode } from '../prover-node.js';

class TestProverNode_ extends ProverNode {
  public override prover!: EpochProverManager;
  public override publisher!: L1TxPublisher;
}

export type TestProverNode = TestProverNode_;
