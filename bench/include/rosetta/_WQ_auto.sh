push_slack_notification() {
    slack_ini_base_path=$2
    slack_ini="_WQ_auto.ini"
    url_endpoint=$(cat $slack_ini_base_path/$slack_ini)
    # url_endpoint=$1
    str_msg=$1
    echo "{\"text\":\"$str_msg\"}"
    curl -X POST -H 'Content-type: application/json' --data "{\"text\":\"$str_msg\"}" $url_endpoint
}