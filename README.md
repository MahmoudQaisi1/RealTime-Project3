# RealTime-Project3:
In this Project it was required to build a simulation of a colony of ants using the concept of multithreading. 
A defined number of ants starts walking in a set of predefined directions (W, E, N, S, NW, NE, SW, SE). 
When an ant smells food within a predefind vacinity it will start moving towards it. While doing so the ant will release phormone 
that will lead other ants to the food. Ants who smell the phermones will also relese phermones but weaker than the original. 

## Approach:
The aproach was to create a shared memory that contains the food. A thread will place a piece of food whithin defined boundries.
Each ant will check this shared memory for food that is close to it. If there is a food within a predefined limit it starts moving towards it.
