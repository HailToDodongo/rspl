/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

function throwError(message, context) {
  throw new Error("Error: " + message + "\n" + JSON.stringify(context));
}

export default { throwError };