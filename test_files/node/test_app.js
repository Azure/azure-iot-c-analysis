'use strict';

const MAX_VALUE = 1000000
const MIN_VALUE = 1

// const unsorted = [992,9,6,3,11,441,2,4,14,1, -1,8,111,100,10000,2000,3000,4000,123,101,345,656, 789,673,234,567,890,112,343,563,133,134,135,136,234235,79890,75,38,8],
function sortingFunction(inputAry)
{
    const len = inputAry.length;
    let count = 0;
    while(count <= len)
    {
        for(let i = 0;i < len-count; i++)
        {
            if(inputAry[i] > inputAry[i+1])
            {
                [inputAry[i], inputAry[i+1]] = [inputAry[i+1], inputAry[i]]
            }
        }
        count++;
    }
    return inputAry;
}

var unsorted = [];
for (var index = 0; index < 100000; index++)
{
    unsorted[index] = Math.random() * (MAX_VALUE - MIN_VALUE) + MIN_VALUE
}

console.log("Sorting array");
sortingFunction(unsorted);

console.log("array Sorted");
