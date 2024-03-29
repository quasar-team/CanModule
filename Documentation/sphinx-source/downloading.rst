===========
Downloading
===========
 
sources
-------
CanModule can be cloned from github, and is integrated in a cmake build chain:

git clone https://github.com/quasar-team/CanModule.git (clones the master/main which is not always frozen)


Use of a specific TAG is highly recommended:
git clone **-b v2.0.22** https://github.com/quasar-team/CanModule.git

  
- check out the `readthedocs`_ and the generated doc from gitlab CI
- the cross OS CI is on `jenkins_can`_ .

.. _jenkins_can: https://ics-fd-cpp-master.web.cern.ch/view/CAN/
.. _readthedocs: https://readthedocs.web.cern.ch/display/CANDev/CAN+development?src=sidebar


dependencies runtime and build
------------------------------
- **linux&windows**: boost (sth like 1.73.0) from `boost`_  . 
   - can be linked statically as well to become a link dependency only.
- **linux**: libsocketcan 
   - git clone `libsocketcan`_ && cmake . && sudo make install
   - can be linked statically as well to become a link dependency only.
- **linux**: socketcan kernel module and the vendor USB drivers
- **linux&windows**: anagate user space library
   - only dynamic library available
- **windows**: systec driver
   - only dynamic library available
- **windows**: peak driver
   - only dynamic library available


.. _libsocketcan: https://gitlab.cern.ch/mludwig/CAN_libsocketcan.git
.. _boost: https://www.boost.org/




