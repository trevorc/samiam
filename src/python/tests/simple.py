from sam import ExecutionState

class SimpleSam:
    def __init__(self, file):
	self.stack = []
	self.heap = []

	for changes in ExecutionState(file):
	    for ch in changes:
		self.buffer_change(ch)
	    self.commit_changes()

    def buffer_change(self, change):
	if change.stack:
	    if change.add:
		self.stack += change.ml
	    elif change.remove:
		del self.stack[-1]
	    else:
		self.stack[change.sa] = change.ml
	else:
	    if change.add:
		self.heap += change.ml
	    elif change.remove:
		del self.heap[-1]
	    else:
		self.heap[change.ha] = change.ml

    def commit_changes(self):
	pass

if __name__ == '__main__':
    import sys
    ss = SimpleSam(sys.argv[1])
