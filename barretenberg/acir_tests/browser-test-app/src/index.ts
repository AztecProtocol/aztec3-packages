import createDebug from "debug";

createDebug.enable("*");
const debug = createDebug("browser-test-app");

async function runTest(
  bytecode: string,
  witness: Uint8Array,
  threads?: number
) {
  const { UltraHonkBackend, BarretenbergVerifier } = await import("@aztec/bb.js");

  debug("starting test...");
  const backend = new UltraHonkBackend(bytecode, { threads });
  const proof = await backend.generateProof(witness);

  debug(`getting the verification key...`);
  const verificationKey = await backend.getVerificationKey();
  debug(`destroying the backend...`);
  await backend.destroy();

  debug(`verifying...`);
  const verifier = new BarretenbergVerifier({ threads });
  const verified = await verifier.verifyUltraHonkProof(proof, verificationKey);
  debug(`verified: ${verified}`);

  await verifier.destroy();

  debug("test complete.");
  return verified;
}

(window as any).runTest = runTest;

function base64ToUint8Array(base64: string) {
  let binaryString = atob(base64);
  let len = binaryString.length;
  let bytes = new Uint8Array(len);
  for (let i = 0; i < len; i++) {
    bytes[i] = binaryString.charCodeAt(i);
  }
  return bytes;
}

// This is the verify_honk_proof test, for triggering via the button click.
// Will likely rot as acir changes.
// Update by extracting from ../acir_tests/verify_honk_proof. Specifically:
//   - The base64 representation of the ACIR is the bytecode section of program.json
//   - The base64 representation of the witness is obtained by encoding witness.gz
const acir =
    "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"
;

const witness =
  base64ToUint8Array(
    "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"
);

document.addEventListener("DOMContentLoaded", function () {
  const button = document.createElement("button");
  button.innerText = "Run Test";
  button.addEventListener("click", () => runTest(acir, witness));
  document.body.appendChild(button);
});