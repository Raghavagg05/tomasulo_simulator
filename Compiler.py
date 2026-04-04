import sys

temp_instructions = []
final_instructions = []
memory = []
memo = {}
labels = {}
memo_counter = 0

with open(sys.argv[1], 'r', encoding='utf-8') as f:
    for line in f:
        # Chop off anything after a '#' symbol to ignore inline comments
        content = line.split('#')[0].strip()
        
        # If the line is empty (or was just a comment), skip it
        if not content: 
            continue
            
        if content.startswith("."): 
            words = content.split()
            memo[words[0][1:-1]] = memo_counter
            values = [int(x) for x in words[1:]]
            memory.extend(values)
            memo_counter += len(values)
        else: 
            temp_instructions.append(content)

for i in range(len(temp_instructions)):
    s = temp_instructions[i]
    ind = s.find(':')
    if ind == -1: 
        final_instructions.append(s)
    else:
        labels[s[0:ind]] = len(final_instructions)
        if ind + 1 != len(s): 
            final_instructions.append(s[ind+1:].strip())

for i in range(len(final_instructions)):
    # Treat commas as spaces so the script is immune to formatting differences
    s = final_instructions[i].replace(',', ' ') 
    parts = [p for p in s.split() if p] # Split and clean empty strings
    
    if len(parts) == 0: continue

    # Fix branch/jump labels
    if parts[0] == 'j' and parts[1] in labels:
        parts[1] = str(labels[parts[1]] - i)
    elif parts[0] in ['beq', 'bne', 'blt', 'ble'] and parts[-1] in labels:
        parts[-1] = str(labels[parts[-1]] - i)
        
    # Fix memory labels (e.g., B(x1))
    elif parts[0] in ['lw', 'sw'] and '(' in parts[-1]:
        ind = parts[-1].find('(')
        cur_label = parts[-1][:ind]
        if cur_label in memo:
            parts[-1] = str(memo[cur_label]) + parts[-1][ind:]
            
    final_instructions[i] = ' '.join(parts)

with open(sys.argv[1], 'w') as f:
    if memory:
        f.write('.DATA ' + ' '.join(str(x) for x in memory) + '\n')
    for inst in final_instructions:
        f.write(inst + '\n')