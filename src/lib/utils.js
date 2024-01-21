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

/**
 * @param {Array} a
 * @param {Array} b
 * @return {boolean}
 */
export function hasIntersection(a, b)
{
  for(const x of a) {
    if(b.includes(x))return true;
  }
  return false;
  //return a.some(x => b.includes(x));
}