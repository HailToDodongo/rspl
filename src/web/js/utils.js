/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export function debounce(func, timeout){
  let timer;
  return (...args) => {
    clearTimeout(timer);
    timer = setTimeout(() => { func.apply(this, args); }, timeout);
  };
}