import { type MerkleTreeWriteOperations, PublicExecutionRequest, type Tx } from '@aztec/circuit-types';
import { CallContext, FunctionSelector, GasFees, GlobalVariables, PUBLIC_DISPATCH_SELECTOR } from '@aztec/circuits.js';
import { type ContractArtifact, encodeArguments } from '@aztec/foundation/abi';
import { AztecAddress } from '@aztec/foundation/aztec-address';
import { Fr } from '@aztec/foundation/fields';
import { AvmTestContractArtifact } from '@aztec/noir-contracts.js/AvmTest';
import { NativeWorldStateService } from '@aztec/world-state';

import { BaseAvmSimulationTester } from '../../avm/fixtures/base_avm_simulation_tester.js';
import { getContractFunctionArtifact, getFunctionSelector } from '../../avm/fixtures/index.js';
import { SimpleContractDataSource } from '../../avm/fixtures/simple_contract_data_source.js';
import { WorldStateDB } from '../public_db_sources.js';
import { type PublicTxResult, PublicTxSimulator } from '../public_tx_simulator.js';
import { createTxForPublicCalls } from './index.js';

const TIMESTAMP = new Fr(99833);
const DEFAULT_GAS_FEES = new GasFees(2, 3);

export type TestEnqueuedCall = {
  address: AztecAddress;
  fnName: string;
  args: any[];
  isStaticCall?: boolean;
};

/**
 * A test class that extends the BaseAvmSimulationTester to enable real-app testing of the PublicTxSimulator.
 * It provides an interface for simulating one transaction at a time and maintains state between subsequent
 * transactions.
 */
export class PublicTxSimulationTester extends BaseAvmSimulationTester {
  private txCount = 0;

  constructor(
    private worldStateDB: WorldStateDB,
    contractDataSource: SimpleContractDataSource,
    merkleTrees: MerkleTreeWriteOperations,
    skipContractDeployments: boolean,
  ) {
    super(contractDataSource, merkleTrees, skipContractDeployments);
  }

  public static async create(skipContractDeployments = false): Promise<PublicTxSimulationTester> {
    const contractDataSource = new SimpleContractDataSource();
    const merkleTrees = await (await NativeWorldStateService.tmp()).fork();
    const worldStateDB = new WorldStateDB(merkleTrees, contractDataSource);
    return new PublicTxSimulationTester(worldStateDB, contractDataSource, merkleTrees, skipContractDeployments);
  }

  public async simulateTx(
    sender: AztecAddress,
    setupCalls: TestEnqueuedCall[] = [],
    appCalls: TestEnqueuedCall[] = [],
    teardownCall?: TestEnqueuedCall,
    feePayer: AztecAddress = AztecAddress.zero(),
  ): Promise<PublicTxResult> {
    const globals = GlobalVariables.empty();
    globals.timestamp = TIMESTAMP;
    globals.gasFees = DEFAULT_GAS_FEES;

    const simulator = new PublicTxSimulator(this.merkleTrees, this.worldStateDB, globals, /*doMerkleOperations=*/ true);

    const setupExecutionRequests: PublicExecutionRequest[] = [];
    for (let i = 0; i < setupCalls.length; i++) {
      const address = setupCalls[i].address;
      const contractArtifact = await this.contractDataSource.getContractArtifact(address);
      const req = await executionRequestForCall(
        sender,
        address,
        setupCalls[i].fnName,
        setupCalls[i].args,
        setupCalls[i].isStaticCall,
        contractArtifact,
      );
      setupExecutionRequests.push(req);
    }
    const appExecutionRequests: PublicExecutionRequest[] = [];
    for (let i = 0; i < appCalls.length; i++) {
      const address = appCalls[i].address;
      const contractArtifact = await this.contractDataSource.getContractArtifact(address);
      const req = await executionRequestForCall(
        sender,
        address,
        appCalls[i].fnName,
        appCalls[i].args,
        appCalls[i].isStaticCall,
        contractArtifact,
      );
      appExecutionRequests.push(req);
    }

    let teardownExecutionRequest: PublicExecutionRequest | undefined = undefined;
    if (teardownCall) {
      const address = teardownCall.address;
      const contractArtifact = await this.contractDataSource.getContractArtifact(address);
      teardownExecutionRequest = await executionRequestForCall(
        sender,
        address,
        teardownCall.fnName,
        teardownCall.args,
        teardownCall.isStaticCall,
        contractArtifact,
      );
    }

    // Use a fake "first nullifier" to make sure note hash nonces are computed properly,
    // but make sure each tx has a unique first nullifier.
    const firstNullifier = new Fr(420000 + this.txCount++);

    const tx: Tx = await createTxForPublicCalls(
      firstNullifier,
      setupExecutionRequests,
      appExecutionRequests,
      teardownExecutionRequest,
      feePayer,
    );

    const startTime = performance.now();
    const avmResult = await simulator.simulate(tx);
    const endTime = performance.now();
    this.logger.debug(`Public transaction simulation took ${endTime - startTime}ms`);

    return avmResult;
  }
}

async function executionRequestForCall(
  sender: AztecAddress,
  address: AztecAddress,
  fnName: string,
  args: Fr[] = [],
  isStaticCall: boolean = false,
  contractArtifact: ContractArtifact = AvmTestContractArtifact,
): Promise<PublicExecutionRequest> {
  const fnSelector = await getFunctionSelector(fnName, contractArtifact);
  const fnAbi = getContractFunctionArtifact(fnName, contractArtifact);
  const encodedArgs = encodeArguments(fnAbi!, args);
  const calldata = [fnSelector.toField(), ...encodedArgs];

  const callContext = new CallContext(
    sender,
    address,
    /*selector=*/ new FunctionSelector(PUBLIC_DISPATCH_SELECTOR),
    isStaticCall,
  );
  return new PublicExecutionRequest(callContext, calldata);
}
