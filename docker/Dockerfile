# Web-simulator dev environment for the RPN app.
#
# Builds the Epsilon web simulator (from a mounted Epsilon checkout) and the RPN
# app (.nwb), then serves them so you can run the app in a browser — no local
# emscripten/node install needed, just Docker.
#
# The emsdk version matches Epsilon's .emsdk-version (4.0.10).
FROM emscripten/emsdk:4.0.10

# Epsilon's asset pipeline and the app build need these on top of emsdk
# (which already ships python3 and node).
RUN apt-get update && apt-get install -y --no-install-recommends \
        git python3 python3-pil python3-lz4 imagemagick pkg-config libfreetype6-dev \
    && rm -rf /var/lib/apt/lists/*

COPY serve.sh /usr/local/bin/serve.sh
RUN chmod +x /usr/local/bin/serve.sh

WORKDIR /work
EXPOSE 8000
ENTRYPOINT ["/usr/local/bin/serve.sh"]
