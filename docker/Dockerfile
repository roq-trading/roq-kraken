FROM condaforge/miniforge3

MAINTAINER Hans Erik Thrane <info@roq-trading.com>

RUN conda install --quiet --yes --channel https://roq-trading.com/conda/unstable roq-kraken

ENTRYPOINT [ "/opt/conda/bin/roq-kraken" ]

CMD [ "--help" ]
