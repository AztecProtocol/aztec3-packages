// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Aztec Labs.
pragma solidity >=0.8.27;

interface IForwarder {
  function forward(address[] calldata _to, bytes[] calldata _data) external;
}
