set(SOURCES
		hackquad/hackquad_main.c
        hackquad/lint_defs.h
        hackquad/pid.h
        hackquad/pid.c
        hackquad/i2c.c
        hackquad/i2c.h
        hackquad/motor.h
        hackquad/motor.c
        hackquad/mpu.h
        hackquad/mpu.c
        hackquad/registry.h
        hackquad/registry.c
        hackquad/udpserver.c
        hackquad/udpserver.h
        hackquad/battery.h
        hackquad/battery.c
        hackquad/wifi.h
        hackquad/wifi.c
        hackquad/hackquad_msg.h
        hackquad/httpserver.h
        hackquad/httpserver.c
        hackquad/blinkcodes.h
        hackquad/blinkcodes.c
		hackquad/flightmath.c
		hackquad/flightmath.h)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ".")