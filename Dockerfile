FROM ubuntu:22.04

# Can avoid prompts while installing things
ENV DEBIAN_FRONTEND=noninteractive 

RUN apt-get update && apt-get install -y gcc iproute2 net-tools iputils-ping && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY coordinate.c hostsfile.txt ./

RUN gcc -o coordinate coordinate.c

ENTRYPOINT ["/app/coordinate"]

CMD ["-h", "hostsfile.txt"]