# Machine setup and deploy scripts for B.A.D

We have three scripts for each stage of setting up a new machine:
* `newMachine.rb` -- Launch a new AWS machine.
* `setupMachine.rb` -- Configure a machine to run B.A.D.
* `deployBAD.rb` -- Deploy a tar.gz dist file to the machine.

And then we have a sript for doing all three in one:
* `launchBAD.rb` -- Launch, configure and deploy B.A.D to a new machine.

## AWS Credentials

You should use the AWS CLI tools to setup your credentials: `aws configure`.

You should have a profile called `bad-project`.

