docker build -t $1 -f ./deploy/arm/Dockerfile . 
docker push $1
