/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export const Log =
{
    set(text) {
        logOutput.innerHTML = text + "\n";
    },

    append(text) {
        logOutput.innerHTML += text + "\n";
    },

    setErrorState(hasError, hasWarn) {
        logOutput.className = hasError ? "error" : (hasWarn ? " warn" : "");
    }
};