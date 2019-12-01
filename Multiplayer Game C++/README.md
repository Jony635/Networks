# SpaceShip.io

## Description

SpaceShip.io is a project developed during the 4th year of the Design and Development of Videogames Bachelor's Degree in CITM (UPC), Terrassa, 2019.
It consist in exploring the basics of networking integrating diferent techniques used in this area, in form of a simple spaceship battle game taking Agar.io as reference.

Each client which join to a server will keep fighting against the other ships until some laser hits her though the server perspective, in which case she will get destroyed and disconnected.
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

- After joining a server, the client will not see the other connected players until they move or shoot the first time.
- The collision detection is calculated server-side without taking in count that the client's world is in an older state than the server one, because packets take time to arrive, so its collision may feel some weird sometimes through the shooter perspective.
