.. _roq-kraken:

.. |dagger| unicode:: U+2020
.. |double-dagger| unicode:: U+2021
.. |right-arrow| unicode:: U+2192
.. |right-double-arrow| unicode:: U+21D2
.. |left-right-double-arrow| unicode:: U+21D4
.. |check-mark| unicode:: U+2705
.. |cross-mark| unicode:: U+274C
.. |negative-cross-mark| unicode:: U+274E
.. |footnote-1| unicode:: U+2776
.. |footnote-2| unicode:: U+2777
.. |footnote-3| unicode:: U+2778


roq-kraken
==========


.. tab:: Unstable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/unstable \
           roq-kraken

.. tab:: Stable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/stable \
           roq-kraken

Supports
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:enumerator:`Spot <roq::SecurityType::SPOT>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Futures <roq::SecurityType::FUTURES>`
        - |cross-mark|
        -
      * - :cpp:enumerator:`Swap <roq::SecurityType::SWAP>`
        - |cross-mark|
        -
      * - :cpp:enumerator:`Option <roq::SecurityType::OPTION>`
        - |cross-mark|
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`ReferenceData <roq::ReferenceData>`
        - |check-mark|
        - |footnote-1|
      * - :cpp:class:`MarketStatus <roq::MarketStatus>`
        - |check-mark|
        - |footnote-1|
      * - :cpp:class:`TopOfBook <roq::TopOfBook>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByPrice <roq::MarketByPriceUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByOrder <roq::MarketByOrderUpdate>`
        - |cross-mark|
        -
      * - :cpp:class:`TradeSummary <roq::TradeSummary>`
        - |check-mark|
        -
      * - :cpp:class:`Statistics <roq::StatisticsUpdate>`
        - |cross-mark|
        -
      * - :cpp:class:`TimeSeries <roq::TimeSeriesUpdate>`
        - |cross-mark|
        -

  .. grid-item-card::  Orders & Quotes

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`CreateOrder <roq::CreateOrder>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`ModifyOrder <roq::ModifyOrder>`
        - |cross-mark|
        -
      * - :cpp:class:`CancelOrder <roq::CancelOrder>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`CancelAllOrders <roq::CancelAllOrders>`
        - |negative-cross-mark|
        -
      * - :cpp:class:`MassQuote <roq::MassQuote>`
        - |cross-mark|
        -
      * - :cpp:class:`CancelQuotes <roq::CancelQuotes>`
        - |cross-mark|
        -

  .. grid-item-card::  Account

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`Funds <roq::FundsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`Position <roq::PositionUpdate>`
        - |cross-mark|
        -

.. note::

   |check-mark| = Available.

   |negative-cross-mark| = Not implemented.

   |cross-mark| = Unavailable.

   |footnote-1| The exchange protocol does not support streaming updates for reference data and market status.


Using
-----

.. code-block:: shell

   $ roq-kraken [FLAGS]


.. _roq-kraken-flags:

Flags
-----

.. code-block:: shell

   $ roq-kraken --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc

.. tab:: Misc

   .. include:: flags/misc.rstinc


Environments
------------

.. tab:: Prod

   .. code-block:: shell

      $ $CONDA_PREFIX/share/roq-kraken/flags/prod/flags.cfg

   .. include:: flags/prod/flags.cfg
     :code: shell

.. tab:: Test

   .. code-block:: shell

      $ $CONDA_PREFIX/share/roq-kraken/flags/test/flags.cfg

   .. include:: flags/test/flags.cfg
     :code: shell


Configuration
-------------

.. code-block:: shell

   $ $CONDA_PREFIX/share/roq-kraken/config.toml

.. important::

   This template will be replaced when the software is upgraded.
   Make a copy and modify to your own needs.

.. include:: config.toml
   :code: toml


Market Data
-----------


Inbound
~~~~~~~


Order Management
----------------


Inbound
~~~~~~~


Outbound
~~~~~~~~


Comments
--------

* Rate-limit usage is not communicated by the exchange

Order book ends up in bad state
  Downtime (scheduled or not) appears to *not* shutdown existing connections *nor* are established
  subscriptions unsubscribed.
  Worse, during downtime, what appears to be uninitialized order book data can be disseminated.
  (This was confirmed with Kraken support early May 2020).

.. warning::
  We currently have no means to detect bad order book updates.
  At best, a parse exception will terminate your gateway with an unhandled excpetion.


References
----------


Common
~~~~~~

* :ref:`Using Conda <tutorial-conda>`
* :ref:`Using Flags <abseil-cpp>`
* :ref:`Gateway Flags <gateway-flags>`
* :ref:`Gateway Config <gateway-config>`


Exchange
~~~~~~~~

* `Website <https://www.kraken.com/>`__
* `Documentation <https://docs.kraken.com/api/>`__
