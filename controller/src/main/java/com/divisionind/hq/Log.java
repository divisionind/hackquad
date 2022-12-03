package com.divisionind.hq;

import java.util.logging.Level;
import java.util.logging.Logger;

public class Log {

    // TODO SimpleDateFormat, LogManager.getLogManager()
    private static final Logger logger = Logger.getLogger(Logger.GLOBAL_LOGGER_NAME);

    public static void info(String msg) {
        logger.log(Level.INFO, msg);
    }

    public static void error(String msg) {
        logger.log(Level.SEVERE, msg);
    }

    public static void warn(String msg) {
        logger.log(Level.WARNING, msg);
    }
}
