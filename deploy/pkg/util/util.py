import shutil
import os
import pathlib

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def copytree(src, dst, symlinks=False, ignore=None):
    for item in src.glob('*'):
        s = src / item.name
        d = dst / item.name
        if s.is_dir():
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)

