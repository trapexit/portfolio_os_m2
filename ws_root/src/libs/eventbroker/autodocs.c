
/******************************************************************************
**
**  @(#) autodocs.c 96/06/27 1.6
**
******************************************************************************/

/**
|||	AUTODOC -class EventBroker -name GetControlPad
|||	Gets control pad events.
|||
|||	  Synopsis
|||
|||	    Err GetControlPad(uint8 padNumber, bool wait,
|||	                      ControlPadEventData *data);
|||
|||	  Description
|||
|||	    This is a convenience call that allows a task to easily monitor events on
|||	    controller pads. This function specifies a controller pad to monitor,
|||	    specifies whether the routine should return immediately or wait until
|||	    something happens on the controller pad, and provides a data structure
|||	    for data from the controller pad.
|||
|||	    When the function executes, it either returns immediately with event
|||	    information or waits until there is a change in the controller pad
|||	    before returning. If an event has occurred, the task must check the
|||	    ControlPadEventData data structure for details about the event.
|||
|||	  Arguments
|||
|||	    padNumber
|||	        Sets the number of the generic controller pad on the control
|||	        port (i.e., the first, second, third, and so on, pad in the
|||	        control port daisy chain, counting out from the 3DO unit) that
|||	        the task wants to monitor. The first pad is 1, the second is 2,
|||	        and so on.
|||
|||	    wait
|||	        If this parameter is TRUE, this function waits until an event
|||	        occurs on the specified pad and only returns with data when
|||	        there is a change in the pad. If it is FALSE, this function
|||	        immediately returns with the status of the pad.
|||
|||	    data
|||	        A pointer to a ControlPadEventData data structure to receive
|||	        the returned control pad data.
|||
|||	  Return Value
|||
|||	    This function returns 1 if an event has occurred on the pad, 0 if no
|||	    event has occurred on the pad, or a negative error code if a
|||	    problem occurred while retrieving an event.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libeventbroker.a V20.
|||
|||	  Associated Files
|||
|||	    <misc/event.h>, libeventbroker.a
|||
|||	  See Also
|||
|||	    InitEventUtility(), GetMouse(), KillEventUtility()
|||
**/

/**
|||	AUTODOC -class EventBroker -name GetMouse
|||	Gets mouse events.
|||
|||	  Synopsis
|||
|||	    Err GetMouse(uint8 mouseNumber, bool wait,
|||	                 MouseEventData *data);
|||
|||	  Description
|||
|||	    This convenience call allows a task to easily monitor mouse events.
|||
|||	    This function is similar to GetControlPad() but gets events from a
|||	    specified mouse instead of a specified controller pad. It specifies
|||	    a mouse to monitor, specifies whether the call should return
|||	    immediately or wait until something happens to the mouse, and
|||	    provides a data structure for data from the mouse.
|||
|||	    When the function executes, it either returns immediately with
|||	    event information or waits there is a change in the mouse before
|||	    returning. If an event has occurred, the task must check the
|||	    MouseEventData data structure for details about the event.
|||
|||	  Arguments
|||
|||	    mouseNumber
|||	        Sets the number of the generic mouse on the control port
|||	        (i.e., the first, second, third, and so on, mouse in the control
|||	        port daisy chain, counting out from the 3DO unit) that the task
|||	        wants to monitor. The first mouse is 1, the second is 2, and so
|||	        on.
|||
|||	    wait
|||	        If this parameter is TRUE, this function waits until an event
|||	        occurs on the specified mouse and only returns with data when
|||	        there is a change in the mouse. If it is FALSE, this function
|||	        immediately returns with the status of the mouse.
|||
|||	    data
|||	        A pointer to a MouseEventData data structure to receive the
|||	        returned mouse data.
|||
|||	  Return Value
|||
|||	    This function returns 1 if an event has occurred on the mouse, 0 if
|||	    no event has occurred on the mouse, or a negative error code if an
|||	    error occurred when attempting to retrieve an event.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libeventbroker.a V20.
|||
|||	  Associated Files
|||
|||	    <misc/event.h>, libeventbroker.a
|||
|||	  See Also
|||
|||	    InitEventUtility(), GetControlPad(), KillEventUtility()
|||
**/

/**
|||	AUTODOC -class EventBroker -name InitEventUtility
|||	Connects the calling task to the event broker.
|||
|||	  Synopsis
|||
|||	    Err InitEventUtility(uint8 numControlPads, uint8 numMice,
|||	                         bool focusListener);
|||
|||	  Description
|||
|||	    This convenience call allows a task to easily monitor events on
|||	    controller pads or mice. This function connects the task to the
|||	    event broker, sets the task's focus interest, and determines how
|||	    many controller pads and mice the task wants to monitor.
|||
|||	    The function creates a reply port and a message, sends a
|||	    configuration message to the event broker (which asks the event
|||	    broker to report appropriate mouse and controller pad events), and
|||	    deals with the event broker's configuration reply.
|||
|||	  Arguments
|||
|||	    numControlPads
|||	        The number of controller pads to monitor.
|||
|||	    numMice
|||	        The number of mice to monitor.
|||
|||	    focusListener
|||	        The focus of the task when it is connected as a listener. If
|||	        the value is TRUE, the task is connected as a focus-dependent
|||	        listener. If the value is FALSE, the task is connected as a
|||	        focus-independent listener.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libeventbroker.a V20.
|||
|||	  Associated Files
|||
|||	    <misc/event.h>, libeventbroker.a
|||
|||	  See Also
|||
|||	    GetControlPad(), GetMouse(), KillEventUtility()
|||
**/

/**
|||	AUTODOC -class EventBroker -name KillEventUtility
|||	Disconnects a task from the event broker.
|||
|||	  Synopsis
|||
|||	    Err KillEventUtility(void);
|||
|||	  Description
|||
|||	    This function disconnects a task that was connected to the event
|||	    broker with the InitEventUtility() function. When executed, the
|||	    function deletes the reply port, and frees all resources used for
|||	    the connection.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libeventbroker.a V20.
|||
|||	  Associated Files
|||
|||	    <misc/event.h>, libeventbroker.a
|||
|||	  See Also
|||
|||	    GetControlPad(), GetMouse(), InitEventUtility()
|||
**/

/* keep the compiler happy... */
extern int foo;
