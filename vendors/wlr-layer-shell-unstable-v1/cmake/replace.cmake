file(READ "${SOURCE}" TEXT)
string(REPLACE "${PATTERN}" "${REPLACE}" TEXT "${TEXT}")
file(WRITE "${OUTPUT}" "${TEXT}")
