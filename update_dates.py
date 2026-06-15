import glob
import re

files = glob.glob('/Users/mihirb/resumekit/output/*.tex')
files.append('/Users/mihirb/resumekit/resume-source/resume_base_mihir.tex') # just in case

for filepath in files:
    try:
        with open(filepath, 'r') as f:
            content = f.read()
            
        # Format 1:
        # \resumeSubheading{Slugbotics, ...}{Jan. 2025 -- Present}
        content, n1 = re.subn(r'(\\resumeSubheading\{Slugbotics[^\}]*\}\{)Jan\.\ 2025 -- Present(\})', r'\g<1>Jan. 2025 -- Jun. 2025\g<2>', content)
        
        # Format 2:
        # \resumeSubheading
        #   {Role}{Jan. 2025 -- Present}
        #   {Slugbotics}{...}
        content, n2 = re.subn(r'(\n\s*\{[^}]*\}\{)Jan\.\ 2025 -- Present(\}\n\s*\{Slugbotics\})', r'\g<1>Jan. 2025 -- Jun. 2025\g<2>', content)

        if n1 > 0 or n2 > 0:
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"Updated {filepath}")
    except Exception as e:
        print(f"Error on {filepath}: {e}")

