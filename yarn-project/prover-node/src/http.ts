import { type ProverNodeApi, ProverNodeApiSchema } from '@aztec/circuit-types';
import { createSafeJsonRpcClient, defaultFetch } from '@aztec/foundation/json-rpc/client';
import { createSafeJsonRpcServer } from '@aztec/foundation/json-rpc/server';

import { type ProverNode } from './prover-node.js';

export { type ProverNodeApi };

/**
 * Wrap a ProverNode instance with a JSON RPC HTTP server.
 * @param node - The ProverNode
 * @returns An JSON-RPC HTTP server
 */
export function createProverNodeRpcServer(node: ProverNode) {
  return createSafeJsonRpcServer(node, ProverNodeApiSchema);
}

export function createProverNodeRpcClient(url: string, namespace: string = '', fetch = defaultFetch): ProverNodeApi {
  return createSafeJsonRpcClient(url, ProverNodeApiSchema, false, namespace, fetch);
}
