import { type AztecNode, type L2BlockNumber } from '@aztec/circuit-types';
import {
  AztecAddress,
  DEPLOYER_CONTRACT_ADDRESS,
  Fr,
  type FunctionSelector,
  type GrumpkinScalar,
  MembershipWitness,
  type NOTE_HASH_TREE_HEIGHT,
  PUBLIC_DATA_TREE_HEIGHT,
  type Point,
  UpdatedClassIdHints,
  VK_TREE_HEIGHT,
  type VerificationKeyAsFields,
  computeContractClassIdPreimage,
  computeSaltedInitializationHash,
} from '@aztec/circuits.js';
import { computePublicDataTreeLeafSlot, deriveStorageSlotInMap } from '@aztec/circuits.js/hash';
import { makeTuple } from '@aztec/foundation/array';
import { poseidon2HashWithSeparator } from '@aztec/foundation/crypto';
import { createLogger } from '@aztec/foundation/log';
import { type Tuple } from '@aztec/foundation/serialize';
import { type KeyStore } from '@aztec/key-store';
import { getVKIndex, getVKSiblingPath } from '@aztec/noir-protocol-circuits-types/vks';

import { type ContractDataOracle } from '../contract_data_oracle/index.js';
import { type ProvingDataOracle } from './../kernel_prover/proving_data_oracle.js';

// TODO: Block number should not be "latest".
// It should be fixed at the time the proof is being simulated. I.e., it should be the same as the value defined in the constant data.
/**
 * A data oracle that provides information needed for simulating a transaction.
 */
export class KernelOracle implements ProvingDataOracle {
  constructor(
    private contractDataOracle: ContractDataOracle,
    private keyStore: KeyStore,
    private node: AztecNode,
    private blockNumber: L2BlockNumber = 'latest',
    private log = createLogger('pxe:kernel_oracle'),
  ) {}

  public async getContractAddressPreimage(address: AztecAddress) {
    const instance = await this.contractDataOracle.getContractInstance(address);
    return {
      saltedInitializationHash: await computeSaltedInitializationHash(instance),
      ...instance,
    };
  }

  public async getContractClassIdPreimage(contractClassId: Fr) {
    const contractClass = await this.contractDataOracle.getContractClass(contractClassId);
    return computeContractClassIdPreimage(contractClass);
  }

  public async getFunctionMembershipWitness(contractClassId: Fr, selector: FunctionSelector) {
    return await this.contractDataOracle.getFunctionMembershipWitness(contractClassId, selector);
  }

  public async getVkMembershipWitness(vk: VerificationKeyAsFields) {
    const leafIndex = await getVKIndex(vk);
    return new MembershipWitness(VK_TREE_HEIGHT, BigInt(leafIndex), await getVKSiblingPath(leafIndex));
  }

  async getNoteHashMembershipWitness(leafIndex: bigint): Promise<MembershipWitness<typeof NOTE_HASH_TREE_HEIGHT>> {
    const path = await this.node.getNoteHashSiblingPath(this.blockNumber, leafIndex);
    return new MembershipWitness<typeof NOTE_HASH_TREE_HEIGHT>(
      path.pathSize,
      leafIndex,
      path.toFields() as Tuple<Fr, typeof NOTE_HASH_TREE_HEIGHT>,
    );
  }

  getNullifierMembershipWitness(nullifier: Fr) {
    return this.node.getNullifierMembershipWitness(this.blockNumber, nullifier);
  }

  async getNoteHashTreeRoot(): Promise<Fr> {
    const header = await this.node.getBlockHeader(this.blockNumber);
    return header.state.partial.noteHashTree.root;
  }

  public getMasterSecretKey(masterPublicKey: Point): Promise<GrumpkinScalar> {
    return this.keyStore.getMasterSecretKey(masterPublicKey);
  }

  public getDebugFunctionName(contractAddress: AztecAddress, selector: FunctionSelector): Promise<string> {
    return this.contractDataOracle.getDebugFunctionName(contractAddress, selector);
  }

  public async getUpdatedClassIdHints(contractAddress: AztecAddress): Promise<UpdatedClassIdHints> {
    const deployerAddress = new AztecAddress(new Fr(DEPLOYER_CONTRACT_ADDRESS));

    const sharedMutableSlot = deriveStorageSlotInMap(new Fr(1), contractAddress);
    const valueChangeSlot = poseidon2HashWithSeparator([sharedMutableSlot], 0);
    const delayChangeSlot = poseidon2HashWithSeparator([sharedMutableSlot], 1);
    const hashSlot = poseidon2HashWithSeparator([sharedMutableSlot], 2);

    const hashLeafSlot = computePublicDataTreeLeafSlot(deployerAddress, hashSlot);
    const updatedClassIdWitness = await this.node.getPublicDataTreeWitness(this.blockNumber, hashLeafSlot);

    if (!updatedClassIdWitness) {
      throw new Error(`No public data tree witness found for ${hashLeafSlot}`);
    }

    const valueChange = makeTuple<Fr, 3>(3, () => Fr.ZERO);
    for (let i = 0; i < 3; i++) {
      const valueChangeItemSlot = valueChangeSlot.add(new Fr(i));
      valueChange[i] = await this.node.getPublicStorageAt(deployerAddress, valueChangeItemSlot, this.blockNumber);
    }

    const delayChange = await this.node.getPublicStorageAt(deployerAddress, delayChangeSlot, this.blockNumber);

    return new UpdatedClassIdHints(
      new MembershipWitness(
        PUBLIC_DATA_TREE_HEIGHT,
        updatedClassIdWitness.index,
        updatedClassIdWitness.siblingPath.toTuple(),
      ),
      updatedClassIdWitness.leafPreimage,
      valueChange,
      [delayChange],
    );
  }
}
