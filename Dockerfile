FROM ubuntu:22.04

# Can avoid prompts while installing things
ENV DEBIAN_FRONTEND=noninteractive 

RUN apt-get update && apt-get install -y gcc

WORKDIR /app

COPY coordinate.c hostsfile.txt ./

RUN gcc -o coordinate coordinate.c

ENTRYPOINT ["/app/coordinate"]

CMD ["-h", "hostsfile.txt"]