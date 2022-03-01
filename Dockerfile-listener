FROM python:3.9-slim-bullseye AS builder

# Set ENV variables that make Python more friendly to running inside a container.
ENV PYTHONDONTWRITEBYTECODE 1

# By default, pip caches copies of downloaded packages from PyPI. These are not useful within
# a Docker image, so disable this to reduce the size of images.
ENV PIP_NO_CACHE_DIR 1
ENV WORKDIR /src

# This must be the same path that is used in the final image as the virtual environment has
# absoulte symlinks in it.
ENV VIRTUAL_ENV /opt/venv

WORKDIR ${WORKDIR}

# Pre-download/compile wheel dependencies into a virtual environment.
# Doing this in a multi-stage build allows ommitting compile dependencies from the final image.
RUN python -m venv ${VIRTUAL_ENV}
ENV PATH "${VIRTUAL_ENV}/bin:${PATH}"

RUN pip install --upgrade pip && pip install paho-mqtt influxdb

## Final Image
# The image used in the final image MUST match exactly to the python_builder image.
FROM python:3.9-slim-bullseye

ENV PYTHONDONTWRITEBYTECODE 1
ENV PIP_NO_CACHE_DIR 1
ENV VIRTUAL_ENV /opt/venv

ENV HOME /home/user
ENV APP_HOME ${HOME}/app

# Create the home directory for the new user.
RUN mkdir -p ${HOME}

# Create the user so the program doesn't run as root. This increases security of the container.
RUN groupadd -r user && \
    useradd -r -g user -d ${HOME} -s /sbin/nologin -c "Docker image user" user

# Setup application install directory.
RUN mkdir ${APP_HOME}

# If you use Docker Compose volumes, you might need to create the directories in the image,
# otherwise when Docker Compose creates them they are owned by the root user and are inaccessible
# by the non-root user. See https://github.com/docker/compose/issues/3270

WORKDIR ${APP_HOME}

# Copy and activate pre-built virtual environment.
COPY --from=builder ${VIRTUAL_ENV} ${VIRTUAL_ENV}
ENV PATH "${VIRTUAL_ENV}/bin:${PATH}"

COPY listener .

# Give access to the entire home folder to the new user so that files and folders can be written
# there. Some packages such as matplotlib, want to write to the home folder.
RUN chown -R user:user ${HOME}

ENTRYPOINT ["python", "-u", "listener"]
# vim: set ft=dockerfile:
