#!/bin/bash

cat <<EOF
fn ops of x is
  x + x - x * x / x;
end

if false do
EOF

for i in `seq 1 50000`
do
  echo "ops($i);"
done

echo end