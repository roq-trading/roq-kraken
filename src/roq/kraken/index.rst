.. _roq-kraken:

.. |checkmark| unicode:: U+2713

roq-kraken
==========

.. important::
  This gateway needs sponsorship to complete certain features.

Links
-----

* `Website <https://www.kraken.com/>`__
* `Status <https://status.kraken.com/>`__
* `Support <https://support.kraken.com/hc/en-us>`__
* `API <https://www.kraken.com/features/api>`__


Purpose
-------

* Maintain network connectivity with the Kraken exchange
* Route exchange updates to connected clients
* Route client requests to the relevant exchange accounts
* Stream all messages to an event-log


Overview
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto

      * - Spot
        - |checkmark|
      * - Futures
        -
      * - Options
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto

      * - Reference Data
        - |checkmark|
      * - Market Status
        -
      * - Top of Book
        - |checkmark|
      * - Market by Price (L2)
        - |checkmark|
      * - Market by Order (L3)
        -
      * - Trade Summary
        - |checkmark|
      * - Statistics
        -

  .. grid-item-card::  Order Management

    .. list-table::
      :widths: auto

      * - Create
        - |checkmark|
      * - Modify
        -
      * - Cancel
        - |checkmark|
      * - Cancel All
        - |checkmark|
      * - Auto Cancellation
        - |checkmark|

  .. grid-item-card::  Account Management

    .. list-table::
      :widths: auto

      * - Positions
        - |checkmark|
      * - Funds
        - |checkmark|

* Data center located in Costa Rica (to be confirmed, best source
  `here <https://bestcoinexchange.com/exchanges/kraken-review/>`__)


Conda
-----

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Install

  .. code-block:: bash

    $ mamba install \
      --channel https://roq-trading.com/conda/stable \
      roq-kraken

.. tab:: Configure

  .. code-block:: bash

    $ cp $CONDA_PREFIX/share/roq-kraken/config.toml $CONFIG_FILE_PATH

    # Then modify $CONFIG_FILE_PATH to match your specific configuration

.. tab:: Run

  .. code-block:: bash

    $ roq-kraken \
          --name "kraken" \
          --config_file "$CONFIG_FILE_PATH" \
          --client_listen_address "$UNIX_SOCKET_PATH" \
          --service_listen_address "$TCP_LISTEN_PORT" \
          --flagfile "$FLAG_FILE"


Config
------

* :ref:`Common Config <gateway-config>`


Flags
-----

* :ref:`Using Flags <abseil-cpp>`
* :ref:`Common Flags <gateway-flags>`

.. code-block:: bash

   $ roq-kraken --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: Common

   .. include:: flags/common.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc


Environments
------------

.. code-block:: bash

  $ $CONDA_PREFIX/share/roq-kraken/flags

.. tab:: Prod

   .. include:: flags/prod/flags.cfg
     :code: ini

.. tab:: Test

   .. include:: flags/test/flags.cfg
     :code: ini


Market Data
-----------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      -
      -
      - Unavailable

    * - :cpp:class:`roq::MarketStatus`
      -
      -
      - Unavailable

    * - :cpp:class:`roq::TopOfBook`
      - MarketData
      - spread
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      - MarketData
      - book
      -

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      - Unavailable

    * - :cpp:class:`roq::TradeSummary`
      - MarketData
      - trade
      -

    * - :cpp:class:`roq::StatisticsUpdate`
      -
      -
      - Not supported

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::ReferenceData`
      - OrderEntry
      - /0/public/AssetPairs
      -

    * - :cpp:class:`roq::MarketStatus`
      -
      -
      -

    * - :cpp:class:`roq::TopOfBook`
      -
      -
      -

    * - :cpp:class:`roq::MarketByPriceUpdate`
      -
      -
      -

    * - :cpp:class:`roq::MarketByOrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeSummary`
      -
      -
      -

    * - :cpp:class:`roq::StatisticsUpdate`
      -
      -
      -

Statistics
~~~~~~~~~~


Order Management
----------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      - DropCopy
      - openOrders
      - Not implemented

    * - :cpp:class:`roq::TradeUpdate`
      - DropCopy
      - ownTrades
      - Not implemented

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderUpdate`
      -
      -
      -

    * - :cpp:class:`roq::TradeUpdate`
      -
      -
      -

.. tab:: Request

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::CreateOrder`
      - DropCopy
      - addOrder
      - Not implemented

    * - :cpp:class:`roq::ModifyOrder`
      -
      -
      - Unavailable

    * - :cpp:class:`roq::CancelOrder`
      - DropCopy
      - cancelOrder
      - Not implemented

    * - :cpp:class:`roq::CancelAllOrders`
      - DropCopy
      - cancelAll
      - Not implemented

.. tab:: Response

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::OrderAck`
      - DropCopy
      - addOrder, cancelOrder
      - Not implemented


Order Types
~~~~~~~~~~~

TBD


Time in Force
~~~~~~~~~~~~~

TBD


Position Effect
~~~~~~~~~~~~~~~

TBD


Execution Instructions
~~~~~~~~~~~~~~~~~~~~~~

TBD


Account Management
------------------

.. tab:: Live

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      -
      -
      -

    * - :cpp:class:`roq::FundsUpdate`
      -
      -
      -

.. tab:: Download

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Event
      - Stream
      - Messages
      - Comments

    * - :cpp:class:`roq::PositionUpdate`
      - OrderEntry
      - /0/private/OpenPositions
      - Not implemented

    * - :cpp:class:`roq::FundsUpdate`
      - OrderEntry
      - /0/private/Balance, /0/private/TradeBalance
      - Not implemented


Streams
-------

.. tab:: OrderEntry

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - REST
      - Primary purpose

        * support order management

        Each connection

        * supports a single account
        * maintains a listen key (used by the DropCopy stream)

        The master account is used to

        * discover the full list of symbols (by downloading asset pairs)

.. tab:: DropCopy

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - WebSocket
      - Primary purpose

        * live account updates, including orders and fills

        Each connection

        * supports a single account

.. tab:: MarketData

  .. list-table::
    :header-rows: 1
    :widths: auto

    * - Type
      - Comments

    * - WebSocket
      - Primary purpose

        * live market data

        Each connection

        * supports a slice of the symbols


Constraints
-----------

* Rate-limit usage is not communicated by the exchange

Order book ends up in bad state
  Downtime (scheduled or not) appears to *not* shutdown existing connections *nor* are established
  subscriptions unsubscribed.
  Worse, during downtime, what appears to be uninitialized order book data can be disseminated.
  (This was confirmed with Kraken support early May 2020).

.. warning::
  We currently have no means to detect bad order book updates.
  At best, a parse exception will terminate your gateway with an unhandled excpetion.

