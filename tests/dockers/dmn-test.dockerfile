FROM debian:stable-slim

WORKDIR /dmn

ADD ./dmn_tests /dmn/
RUN chmod +x ./dmn*

CMD ./dmn_tests --log_level=test_suite
