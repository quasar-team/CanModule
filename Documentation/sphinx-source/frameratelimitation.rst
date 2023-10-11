frame rate limitation
=====================

For a sustained high frame rate on the BUS, especially in combination with a low bitrate, long cables and many nodes, the probability for (standard) collissions and (standard) arbitration increases. When an arbitration occurs, the highest ID frame wins, and the lower ID frame(s) get retransmitted after a short time.

If the conditions on a bus are tight enough there will not be enough time for all retransmissions to succeed, internal (hardware) buffers will start to overflow and frames get lost.

The frame losses can be measured as a function of bitrate, implementation, module type, firmware version and of course number of nodes and frame sending rate from each node, in the lab test bench. A maximum frame rate frequency where no frame losses are (yet) occurring can be found, for each bitrate and implementation. This maximal frame rate should be quite conservative in order to ensure no frame losses for continuous transmissions: the delays associated to these lossless limits are named "reference delays" in CanModule.

Since every installation is different, no single minimal delay (max. frame rate) can be properly established. Therefore a factor, which is multiplied with the reference delay, can be specified. This factor has to be etimated and established by experience (and possibly a measurement) for a given installation. 

The default for the delay is 0, meaning "as high as possible" for the frame rate, making the frame rate also CPU load dependent. This is the general situation until 2023, and it works for "not too much load" on the bus and shorter bursts with less than 1000 frames.

The default situation can be improved and defined better by using a delay: for shorter bursts this will introduce a small communication speed penalty and for higher and sustained framerates the probability of frame losses is considerably reduced, at the price of a slower communication.


.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: createBus


