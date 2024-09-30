FROM golang:1.17-alpine

WORKDIR /app

# Copy the Go module files
COPY go.mod ./
# COPY go.sum ./

# Download dependencies
# RUN go mod download

# Copy the source code into the container
COPY . .

# Build the Go application
RUN go build -o coordinate

# Start
ENTRYPOINT ["/app/coordinate"]