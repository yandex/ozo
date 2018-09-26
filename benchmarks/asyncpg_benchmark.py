#!/usr/bin/env python3

import asyncio
import asyncpg
import time
import sys


def main():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run())


async def run():
    total_rows = [0]
    start = time.monotonic()
    await asyncio.gather(*[reuse_connection(total_rows) for _ in range(CONNECTIONS)])
    finish = time.monotonic()
    print('read %s rows, %.3f row/sec' % (total_rows[0], total_rows[0] / (finish - start)))


async def reuse_connection(total_rows):
    conn = await asyncpg.connect(sys.argv[1])
    while total_rows[0] < 10000000:
        values = await conn.fetch(QUERY, -1, True)
        total_rows[0] += len(values)
    await conn.close()


CONNECTIONS = 8
QUERY = '''
SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory,
       typispreferred, typisdefined, typdelim, typrelid, typelem, typarray
  FROM pg_type
 WHERE typtypmod = $1::int AND typisdefined = $2::boolean
'''

if __name__ == '__main__':
    main()
