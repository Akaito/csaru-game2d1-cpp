TODO
====

1. Make ObjectDb; has many ObjectDbTable (one per type).
	- ObjectDbTables register themselves with an ObjectDb with a module/typename.
	- Level stuff loading then has data objects directed to one of these tables
		(they'll need a virtual interface or callback for this).

