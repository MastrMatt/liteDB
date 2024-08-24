# create 

# Use an official Ubuntu base image
FROM ubuntu:latest

#Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Set the working directory
WORKDIR /app

#Install build dependencies in the working directory
RUN apt-get update && apt-get install -y \
    build-essential  && \
    rm -rf /var/lib/apt/lists/*

# Copy the source code into the working directory
COPY . .

# Compile the project
RUN make all

# Run the client
CMD ["./runclient"]

