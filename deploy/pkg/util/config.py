import pathlib
import collections
import yaml
import re
import os
import time
import random
import string

# Global configuration (cfCtx set by initialize())
ctx = None

# These represent all available user-defined options (those set by the
# environment or config files). See default-config.yaml or the documentation
# for the meaning of these options.
userOpts = [
        'chipyard-dir',
        'log-dir'
    ]

# These represent all available derived options (constants and those generated
# from userOpts, but not directly settable by users)
derivedOpts = [
        'cf-dir',           # top of centrifuge directory tree
        'support-dir',      # Various supporting non-code files (e.g. context configs)
        'template-dir',      # Various supporting non-code files (e.g. code templates)
        'genhw-dir',        # where to put generated hardware
        'run-name'          # A unique name for a single invocation of this command
    ]

# These options represent userOpts and derivedOpts that should be pathlib paths.
pathOpts = [
    'chipyard-dir',
    'log-dir',
    'cf-dir',
    'genhw-dir'
]

class ConfigurationError(Exception):
    """Error representing a generic problem with configuration"""
    def __init__(self, cause):
        self.cause = cause

    def __str__(self):
        return "Configuration Error: " + self.cause

class ConfigurationOptionError(ConfigurationError):
    """Error representing a problem with a specific configuration option."""
    def __init__(self, opt, cause):
        self.opt = opt
        self.cause = cause

    def __str__(self):
        return "Error with configuration option '" + self.opt + "': " + str(self.cause)
        
class ConfigurationFileError(ConfigurationError):
    """Error representing issues with loading the configuration"""
    def __init__(self, missingFile, cause):
        self.missingFile = missingFile
        self.cause = cause

    def __str__(self):
        return "Failed to load configuration file: " + str(self.missingFile) + "\n" + \
                str(self.cause)

def cleanPaths(opts, baseDir=pathlib.Path('.')):
    """Clean all user-defined paths in an options dictionary by converting them
    to resolved, absolute, pathlib.Path's. Paths will be interpreted as
    relative to baseDir."""

    for opt in pathOpts:
        if opt in opts and opts[opt] is not None:
            try:
                path = (baseDir / pathlib.Path(opts[opt])).resolve(strict=True)
                opts[opt] = path
            except Exception as e:
                raise ConfigurationOptionError(opt, "Invalid path: " + str(e))


class cfCtx(collections.MutableMapping):
    """Global Centrifuge context (configuration)."""
    
    # Actual internal storage for all options
    opts = {}

    def __init__(self):
        """On init, we search for and load all sources of options.

        The order in which options are added here is the order of precidence.
        
        Attributes:
            opts: Dictonary containing all configuration options (static values
                set by the user or statically derived from those). Option
                values are documented in the package variables 'derivedOpts' and
                'userOpts'
        """

        # These are set early to help with config file search-paths
        self['cf-dir'] = pathlib.Path(__file__).parent.parent.parent.parent.resolve()
        self['support-dir'] = self['cf-dir'] / 'deploy' / 'support'
        self['template-dir'] = self['cf-dir'] / 'deploy' / 'templates'

        # This is an exhaustive list of defaults, it always exists and can be
        # overwritten by other user-defined configs
        defaultCfg = self['support-dir'] / 'default-config.yaml'
        self.addPath(defaultCfg)
        
        # These are mutually-exlusive search paths (only one will be loaded)
        cfgSources = [
            # pwd
            pathlib.Path('cf-config.yaml'),
            self['cf-dir'] / 'cf-config.yaml'
        ]
        for src in cfgSources:
            if src.exists():
                self.addPath(src)
                break

        self.addEnv()

        # We should have all user-defined options now
        missingOpts = set(userOpts) - set(self.opts)
        if len(missingOpts) != 0:
            raise ConfigurationError("Missing required options: " + str(missingOpts))

        self.deriveOpts()

        # It would be a marshal bug if any of these options are missing
        missingDOpts = set(derivedOpts) - set(self.opts)
        if len(missingDOpts) != 0:
            raise RuntimeError("Internal error: Missing derived options or constants: " + str(missingDOpts))

    def add(self, newOpts):
        """Add options to this configuration, opts will override any
        conflicting options.
        
        newOpts: dictionary containing new options to add"""
        
        self.opts = dict(self.opts, **newOpts)
        
    def addPath(self, path):
        """Add the yaml file at path to the config."""

        try:
            with open(path, 'r') as newF:
                newCfg = yaml.full_load(newF)
        except Exception as e:
            raise ConfigurationFileError(path, e)

        cleanPaths(newCfg, baseDir=path.parent)
        self.add(newCfg)

    def addEnv(self):
        """Find all options in the environment and load them.
        
        Environment options take the form CF_OPT where "OPT" will be
        converted as follows:
            1) convert to lower-case
            2) all underscores will be replaced with dashes

        For example CF_GENHW_DIR=../special/generators would add a ('genhw-dir'
        : '../special/generators') option to the config."""

        reOpt = re.compile("^CF_(\S+)")
        envCfg = {}
        for opt,val in os.environ.items():
            match = reOpt.match(opt)
            if match:
                optName = match.group(1).lower().replace('_', '-')
                envCfg[optName] = val

        cleanPaths(envCfg)
        self.add(envCfg)

    def deriveOpts(self):
        """Update or initialize all derived options. This assumes all
        user-defined options have been set already. See the 'derivedOpts' list
        above for documentation of these options."""
        self['genhw-dir'] = self['chipyard-dir'] / 'generators' 
        self['run-name'] = ""

    def setRunName(self, configPath, operation):
        """Helper function for formatting a  unique run name. You are free to
        set the 'run-name' option directly if you don't need the help.

        Args:
            configPath (pathlike): Config file used for this run
            operation (str): The operation being performed on this run (e.g. 'build') 
        """

        if configPath:
            configName = pathlib.Path(configPath).stem
        else:
            configName = ''

        timeline = time.strftime("%Y-%m-%d--%H-%M-%S", time.gmtime())
        randname = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(16))

        runName = configName + \
                "-" + operation + \
                "-" + timeline + \
                "-" +  randname

        self['run-name'] = runName

    # The following methods are needed by MutableMapping
    def __getitem__(self, key):
        if key not in self.opts:
            raise ConfigurationOptionError(key, 'No such option')

        return self.opts[key]

    def __setitem__(self, key, value):
        self.opts[key] = value

    def __delitem__(self, key):
        del self.opts[key]

    def __iter__(self):
        return iter(self.opts)

    def __len__(self):
        return len(self.opts)

    def __str__(self):
        return pprint.pformat(self.opts)

    def __repr__(self):
        return repr(self.opts)

def initConfig():
    """Initialize the global configuration. You must call this before calling getOpt()."""
    global ctx
    ctx = cfCtx()
    return ctx

# The following two functions exist to work around how python handles globals.
# Without an explicit getter, other modules that import config.py would only
# ever get a stale reference to ctx
def getCtx():
    """Return the global confguration object (ctx). This is only valid after
    calling initialize().
    
    Returns (cfCtx)
    """ 
    return ctx

def getOpt(opt):
    """Convenience function for reading global configuration options."""
    if ctx is None:
        raise RuntimeError("Context not initized")
    else:
        return ctx[opt]
