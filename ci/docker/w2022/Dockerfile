FROM registry.cern.ch/mcr.microsoft.com/windows/server:ltsc2022

COPY ./ci/docker/w2022/* ./

RUN powershell .\install.ps1
