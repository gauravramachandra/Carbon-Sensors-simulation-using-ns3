# Wrapper to run from this project folder
import os, subprocess, sys
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, '..'))
os.chdir(root)
subprocess.check_call([sys.executable, os.path.join(root, 'visualize_carbon_data.py')])
