gcloud compute disks create client-disk-test --source-snapshot=quhang-linearscan-test-2 --zone us-central1-f
gcloud compute instances create client-test --zone us-central1-f --disk name=client-disk-test,boot=yes,auto-delete=yes --local-ssd --machine-type n1-highmem-32
for i in {1,2,3,4};
do
	gcloud compute disks create node-disk-test-${i} --source-snapshot=quhang-linearscan-test-2 --zone us-central1-f
	gcloud compute instances create node-test-${i} --zone us-central1-f --disk name=node-disk-test-${i},boot=yes,auto-delete=yes --local-ssd --machine-type n1-highmem-4
done
