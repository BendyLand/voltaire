#!/usr/bin/env python3
import sys
from importlib.util import module_from_spec, spec_from_file_location
from types import ModuleType

# Helper module loader (keeps helper modules like methods.py available)
def _helper_module(name, path):
    spec = spec_from_file_location(name, path)
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.modules[name] = module

_helper_module("methods", "methods.py")
import methods

# Mock 'env' object so old sc.env references don't instantly crash Python on import
class DummyEnv:
    def __getattr__(self, name):
        return lambda *args, **kwargs: None
    def __getitem__(self, item):
        return []

env = DummyEnv()

def Import(*args, **kwargs):
    pass
