import glob

files = glob.glob('/Users/mihirb/resumekit/output/*.tex')
files.append('/Users/mihirb/resumekit/output/resume_base_mihir.tex') 

for filepath in files:
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        old_str1 = r'\href{https://www.linkedin.com/in/mihir-bhagatwala/}{https://www.linkedin.com/in/mihir-bhagatwala/}'
        new_str1 = r'\href{https://mihirb-hub.github.io/Portfolio/}{mihirb-hub.github.io/Portfolio}'
        
        old_str2 = r'\href{https://www.linkedin.com/in/mihir-bhagatwala/}{linkedin.com/in/mihir-bhagatwala}'
        
        updated = False
        if old_str1 in content:
            content = content.replace(old_str1, new_str1)
            updated = True
        if old_str2 in content:
            content = content.replace(old_str2, new_str1)
            updated = True
            
        if updated:
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"Updated {filepath}")
    except Exception as e:
        print(f"Failed on {filepath}: {e}")

