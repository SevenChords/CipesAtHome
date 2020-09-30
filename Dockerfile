# The new base image to contain common runtime dependencies
FROM alpine:edge AS base

RUN apk update \
    && apk add --no-cache \
    libgomp

WORKDIR /app

# The first stage will install build dependencies on top of the
# runtime dependencies, and then compile
FROM base AS builder

RUN apk update \
    && apk add --no-cache \
    gcc \
    make \
    musl-dev \
    curl-dev \
    libconfig-dev

COPY . .

RUN make

# The second stage will already contain all dependencies, just copy
# the compiled executables
FROM base

RUN apk update \
    && apk add --no-cache \
    libcurl \
    libconfig

COPY --from=builder /app/recipesAtHome /app/

CMD [ "/app/recipesAtHome" ]
