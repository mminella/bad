# Shuffle All: Experiment 2

Client   : i2.8xlarge
Nodes    : i2.1xlarge
Data     : 600GB
Operation: ReadAll

## Data / Logs

See `experiments/meth4-logs/i2.xlarge-Nnodes`.

Nodes | Predicted | Observed | Accurracy | Error
------------------------------------------------
  4   |    32     |    31    |   96.0%   | 4.2%
  5   |    27     |    26    |   96.2%   | 4.0%
  6   |    23     |    23    |   97.6%   | 2.5%
  8   |    18     |    18    |   97.3%   | 2.8%
 10   |    15     |    14    |   97.4%   | 2.7%

