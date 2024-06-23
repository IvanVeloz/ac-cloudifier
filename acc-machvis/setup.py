from setuptools import setup

setup(
    name='acc-machvis',
    version='0.1.1',
    description='acc-machvis',
    author='Ivan Veloz',
    url='https://www.github.com/IvanVeloz/ac-cloudifier/',
    py_modules=['accvis', 'machvis', 'fakevis'],
    entry_points={
        'console_scripts': [
            'acc-machvis=machvis:main',
            'acc-fakevis=fakevis:main',
        ],
    },
    install_requires=[
        'numpy>=1.24.4',
        'opencv-python>=4.8.0',
    ],
    python_requires='>=3.8.10',
)
