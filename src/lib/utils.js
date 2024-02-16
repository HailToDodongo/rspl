/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

/**
 * @param {Array} a
 * @param {Array} b
 * @returns {Array}
 */
export function intersect(a, b)
{
  return a.filter(x => b.includes(x));
}

/**
 * @param {Array} a
 * @param {Array} b
 * @returns {Array}
 */
export function difference(a, b)
{
  return a.filter(x => !b.includes(x));
}

export async function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}