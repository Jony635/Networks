# SpaceShip.io

## Description

SpaceShip.io is a project developed during the 4th year of the Design and Development of Videogames Bachelor's Degree in CITM (UPC), Terrassa, 2019.
It consists of exploring the basics of networking integrating different techniques used in this area, in form of a simple spaceship battle game taking Agar.io as reference.

Each client which joins a server will keep fighting against other ships until a laser hits it through the server perspective, in which case it will get destroyed and the client disconnected.
For each ship you destroy you will be rewarded with some points to keep climbing in the Score Rank.

We are FlanStudio, a team composed by two students: Jonathan Molina-Prados Ciudad and Oriol de Dios Miranda.

## Features

- The game accepts as much clients as the server machine can support.
- The game handles join/leave events during games without problems.
- The game has a world state replication system properly implemented in order to all the clients see the same in their worlds.

Optional features implemented:

- The game uses the redundant sending of input packets technique to improve on packet loss ratios.
- The game uses a Delivery Manager which manages the packets dedicated to the world replication, and ensures everything arrives correctly to all the clients.
- The game uses client-side prediction, to avoid laggy user input.
- The game uses entity interpolation, to make network objects update smoothly without waiting to the server response.

## Known Issues

- As the server spawns the laser, not the client, they are delayed through the client perspective.
- The collision detection is calculated server-side without taking in count that the client's world is in an older state than the server one, because packets take time to arrive, so its collision may feel some weird sometimes through the shooter perspective.
