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

# Define port environtment variable
ENV port=9255

# Expose the docker port
EXPOSE 9255

# Run the client
CMD ["./runclient"]

