===========
Downloading
===========
 
sources
-------
CanModule can be cloned from github, and is integrated in a cmake build chain:

git clone https://github.com/quasar-team/CanModule.git

or a specific tag
git clone **-b v2.0** https://github.com/quasar-team/CanModule.git

  
- check out the `readthedocs`_ and the generated doc from gitlab CI
- the cross OS CI is on `jenkins_can`_ .

.. _jenkins_can: https://ics-fd-cpp-master.web.cern.ch/view/CAN/
.. _readthedocs: https://readthedocs.web.cern.ch/display/CANDev/CAN+development?src=sidebar


runtime dependencies
--------------------
- boost 1.73.0 from `boost`_
- libsocketcan for linux. 
  git clone `libsocketcan`_ && cmake . && sudo make install 

- anagate lib
- systec driver
- peak driver

you can also build a reduced dependency CanModule with boost and libsocketcan linked statically.

.. _libsocketcan: https://gitlab.cern.ch/mludwig/CAN_libsocketcan.git
.. _boost: https://www.boost.org/




