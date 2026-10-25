from setuptools import find_packages
from setuptools import setup

setup(
    name='gb_interfaces',
    version='0.0.0',
    packages=find_packages(
        include=('gb_interfaces', 'gb_interfaces.*')),
)
