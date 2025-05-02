## Boostraping the database

You can prepare your Underpass installation with data for a specific country.

### Pre-requisites

#### Database

Prepare your PostgreSQL + PostGIS database, for example:

```
sudo apt update 
sudo apt install postgis
sudo su - postgres
psql
postgres=# CREATE USER underpass WITH PASSWORD 'your_password';
postgres=# CREATE DATABASE underpass;
postgres=# GRANT ALL PRIVILEGES ON DATABASE "underpass" to underpass;
postgres=# ALTER ROLE underpass SUPERUSER;
postgres=# exit
```

#### Requirements poly2geojson.py

```
sudo apt install python3-pip -y
sudo apt install python3.11-venv
python3 -m venv ~/venv
source ~/venv/bin/activate
pip install fiona
pip install shapely
```

### Run

```bash
mkdir test && cd test
wget https://download.geofabrik.de/europe/andorra-latest.osm.pbf
wget https://download.geofabrik.de/europe/andorra.poly
python ../utils/poly2geojson.py andorra.poly
underpass -i andorra-latest.osm.pbf -s localhost/underpass -b andorra.geojson
```

