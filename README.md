# Federated_Learning_on_FPGA_cluster
Federated Learning on a cluster of PynqZ2 FPGA boards

Traditionally, Machine Learning model training is carried out in a centralized manner, where the edge devices (nodes) need to send all their data to a central server. This centralized training method raises a major concern about data privacy since all data leaves the nodes and has to go to the server. Moreover, the latency associated with the movement of such huge volumes of training data is very large, and so is the energy cost associated with it. Federated Learning addresses these issues through a distributed and secure training method.

Federated Learning essentially means training a centralized model in a decentralized manner. In this paradigm, instead of moving all data from nodes to a central server, the training of the model is distributed between individual nodes. Let us now understand the steps in how this is done. The server first sends the model to all the edge devices (nodes). The model is partially trained on each node using the local data present on that particular node. So, data stays where it is, and does its part in partial training of the overall model. All the nodes then send the partially updated models to the central server, where secure aggregation of the model parameters is performed to form a global model trained with data from all the nodes. This completes a single iteration of training. For another successive iteration, the server sends back the aggregated model to all the nodes. The nodes again perform partial updation of the model parameters based on their local data. Several such iterations are done in order to train the overall model to achieve good accuracy. Hence, Federated Learning not only preserves data privacy but also reduces communication overhead because transferring model parameters is much cheaper than transferring entire training data.

In tis project, multiple PynqZ2 FPGA boards act as our edge nodes.
