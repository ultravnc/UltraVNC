"""
Setup script for librats Python bindings.
"""

from setuptools import setup, find_packages
import os

# Read the README file
def read_readme():
    with open(os.path.join(os.path.dirname(__file__), 'README.md'), 'r', encoding='utf-8') as f:
        return f.read()

# Read requirements
def read_requirements():
    requirements = []
    try:
        with open(os.path.join(os.path.dirname(__file__), 'requirements.txt'), 'r') as f:
            requirements = [line.strip() for line in f if line.strip() and not line.startswith('#')]
    except FileNotFoundError:
        pass
    return requirements

setup(
    name='librats-py',
    version='1.0.0',
    author='librats contributors',
    author_email='',
    description='Python bindings for librats P2P networking library',
    long_description=read_readme(),
    long_description_content_type='text/markdown',
    url='https://github.com/your-org/librats',
    project_urls={
        'Documentation': 'https://your-org.github.io/librats',
        'Source': 'https://github.com/your-org/librats',
        'Tracker': 'https://github.com/your-org/librats/issues',
    },
    packages=find_packages(),
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Topic :: Internet',
        'Topic :: System :: Networking',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    python_requires='>=3.7',
    install_requires=read_requirements(),
    extras_require={
        'dev': [
            'pytest>=6.0',
            'pytest-asyncio',
            'pytest-cov',
            'black',
            'flake8',
            'mypy',
        ],
        'examples': [
            'asyncio',
            'click',
        ],
    },
    include_package_data=True,
    package_data={
        'librats_py': ['*.so', '*.dll', '*.dylib'],
    },
    zip_safe=False,
    keywords='p2p networking nat-traversal file-transfer gossipsub dht',
)
