"""\
See `StreamLogger`.

This is taken from https://gist.github.com/pmuller/2376336
which has no license associated with it.

"""
import sys
import logging
import io
from time import sleep, strftime, gmtime
import random
import string

def setup_logging(module_name, logger):
    # logging setup
    def logfilename():
        """ Construct a unique log file name from: date + 16 char random. """
        timeline = strftime("%Y-%m-%d--%H-%M-%S", gmtime())
        randname = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(16))
        return module_name + "-" + timeline + "-" + randname + ".log"
        
    # log to file
    full_log_filename = logfilename()

    fileHandler = logging.FileHandler(full_log_filename)
    # formatting for log to file
    # TODO: filehandler should be handler 0 (firesim_topology_with_passes expects this
    # to get the filename) - handle this more appropriately later
    logFormatter = logging.Formatter("%(asctime)s [%(funcName)-12.12s] [%(levelname)-5.5s]  %(message)s")
    fileHandler.setFormatter(logFormatter)
    fileHandler.setLevel(logging.NOTSET) # log everything to file
    logger.addHandler(fileHandler)

    # log to stdout, without special formatting
    consoleHandler = logging.StreamHandler(stream=sys.stdout)
    consoleHandler.setLevel(logging.INFO) # show only INFO and greater in console
    logger.addHandler(consoleHandler)
    

class StreamLogger(object):
    """
    A helper which intercepts what's written to an output stream
    then sends it, line by line, to a `logging.Logger` instance.
    Usage:
        By overwriting `sys.stdout`:
            sys.stdout = StreamLogger('stdout')
            print 'foo'
        As a context manager:
            with StreamLogger('stdout'):
                print 'foo'
    """

    def __init__(self, name, logger=None, unbuffered=False,
                 flush_on_new_line=True):
        """
        ``name``: The stream name to incercept ('stdout' or 'stderr')
        ``logger``: The logger that will receive what's written to the stream.
        ``unbuffered``: If `True`, `.flush()` will be called each time
                        `.write()` is called.
        ``flush_on_new_line``: If `True`, `.flush()` will be called each time
                               `.write()` is called with data containing a
                               new line character.
        """
        self.__name = name
        self.__stream = getattr(sys, name)
        self.__logger = logger or logging.getLogger()
        self.__buffer = io.StringIO()
        self.__unbuffered = unbuffered
        self.__flush_on_new_line = flush_on_new_line

    def write(self, data):
        """Write data to the stream.
        """
        self.__buffer.write(data)
        if self.__unbuffered is True or \
           (self.__flush_on_new_line is True and '\n' in data):
            self.flush()

    def flush(self):
        """Flush the stream.
        """
        self.__buffer.seek(0)
        while True:
            line = self.__buffer.readline()
            if line:
                if line[-1] == '\n':
                    line = line[:-1]
                    if line:
                        level, line = self.parse(line)
                        logger = getattr(self.__logger, level)
                        logger(line)
                else:
                    self.__buffer.seek(0)
                    self.__buffer.write(line)
                    self.__buffer.truncate()
                    break
            else:
                self.__buffer.seek(0)
                self.__buffer.truncate()
                break

    def parse(self, data):
        """Override me!
        """
        return 'debug', data

    def isatty(self):
        """I'm not a tty.
        """
        return False

    def __enter__(self):
        """Enter the context manager.
        """
        setattr(sys, self.__name, self)

    def __exit__(self, exc_type, exc_value, traceback):
        """Leave the context manager.
        """
        setattr(sys, self.__name, self.__stream)


class InfoStreamLogger(StreamLogger):
    """ StreamLogger, but write to info log instead of debug. """

    def parse(self, data):
        return 'info', data
