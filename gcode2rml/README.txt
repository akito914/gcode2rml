
gcode2rml - G-code to RML-1 converter

Convert NC code to RML-1 format for use with Roland MDX-20.


The following code is appended at the beginning of the output code.
;;^IN;V85.0;^PR;Z0,0,15500;^PA;

The following code is appended at the end of the output code.
^IN;


# Supported NC code
- G00 Rapid positioning
- G01 Linear interpolation
- G02 Circular interpolation, clockwise
- G03 Circular interpolation, counterclockwise
- G17 XY plane selection
- G18 XZ plane selection
- G28 YZ plane selection
- G90 Absolute programming
- G91 Incremental programming
- M03 Spindle CW
- M05 spindle Stop

Unsupported NC code will be ignored

