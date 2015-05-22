// Copyright (c) 2015, Galaxy Authors. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "sdk/galaxy.h"

#include <cstdio>
#include <cstdlib>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <gflags/gflags.h>



DECLARE_string(master_addr);
DECLARE_string(agent_addr);
DECLARE_string(job_name);
DECLARE_string(task_raw);
DECLARE_string(cmd_line);
DECLARE_int32(replicate_num);
DECLARE_double(cpu_soft_limit);
DECLARE_double(cpu_limit);
DECLARE_int32(deploy_step_size);
DECLARE_bool(one_task_per_host);
DECLARE_int64(task_id);
DECLARE_int64(job_id);
DECLARE_int64(mem_gbytes);
DECLARE_string(restrict_tag);
DECLARE_string(tag);
DECLARE_string(agent_list);

std::string USAGE = "./galaxy_client add --master_addr=localhost:9527 --job_name=1234 ....\n" 
                   "./galaxy_client list --task_id=1234 \n"
                   "./galaxy_client list --job_id=9527\n"
                    "./galaxy_client kill --job_id=9527";

int ProcessNewJob(){
    if(FLAGS_job_name.empty()){
        fprintf(stderr, "--job_name or -job_name option which can not be empty  is required ");
        return -1;
    }
    if(FLAGS_task_raw.empty()){
        fprintf(stderr, "--package  option which can not be empty  is required ");
        return -1;
    }
    if(FLAGS_cmd_line.empty()){
        fprintf(stderr, "--cmd_line  option which can not be empty  is required ");
        return -1;
    }
    std::string task_raw;
    if (!boost::starts_with(FLAGS_task_raw, "ftp://")) {
        FILE* fp = fopen(FLAGS_task_raw.c_str(), "r");
        if (fp == NULL) {
            fprintf(stderr, "Open %s for read fail\n", FLAGS_task_raw.c_str());
            return -2;
        }
        char buf[1024];
        int len = 0;
        while ((len = fread(buf, 1, 1024, fp)) > 0) {
            task_raw.append(buf, len);
        }
        fclose(fp);
        printf("Task binary len %lu\n", task_raw.size());
    }
    else {
        task_raw = FLAGS_task_raw;
    }
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    galaxy::JobDescription job;
    galaxy::PackageDescription pkg;
    pkg.source = task_raw;
    job.pkg = pkg;
    job.cmd_line = FLAGS_cmd_line;
    job.replicate_count = FLAGS_replicate_num;
    job.job_name = FLAGS_job_name;
    job.cpu_share = FLAGS_cpu_soft_limit;
    job.mem_share = 1024 * 1024 * 1024 * FLAGS_mem_gbytes;
    job.deploy_step_size = FLAGS_deploy_step_size;
    job.cpu_limit = FLAGS_cpu_limit;
    job.one_task_per_host = FLAGS_one_task_per_host;
    job.restrict_tag = FLAGS_restrict_tag;
    fprintf(stdout, "%ld", galaxy->NewJob(job));
    return 0;
}

int ListTask(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    galaxy->ListTask(FLAGS_job_id, FLAGS_task_id, NULL);
    return 0;
}

int ListJob(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    std::vector<galaxy::JobInstanceDescription> jobs;
    galaxy->ListJob(&jobs);
    std::vector<galaxy::JobInstanceDescription>::iterator it = jobs.begin();
    fprintf(stdout, "================================\n");
    for(;it != jobs.end();++it){
        fprintf(stdout, "%ld\t%s\t%d\t%d\n",
                it->job_id, it->job_name.c_str(),
                it->running_task_num, it->replicate_count);
    }
    return 0;
}

int ListNode(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    std::vector<galaxy::NodeDescription> nodes;
    galaxy->ListNode(&nodes);
    std::vector<galaxy::NodeDescription>::iterator it = nodes.begin();
    fprintf(stdout, "================================\n");
    for(; it != nodes.end(); ++it){
        fprintf(stdout, "%ld\t%s\tTASK:%d\tCPU:%0.2f\t"
                    "USED:%0.2f\tMEM:%ldGB\tUSED:%ldGB\tTAG:%s\n",
                    it->node_id, it->addr.c_str(),
                    it->task_num, it->cpu_share,
                    it->cpu_used, it->mem_share/(1024*1024*1024),
                    it->mem_used/(1024*1024*1024),
                    boost::algorithm::join(it->tags, ",").c_str());
    }
    return 0;
}
int ListTaskByAgent(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    if(FLAGS_agent_addr.empty()){
        fprintf(stderr, "--agent_addr which can not be empty  is required ");
        return -1;
    }
    galaxy->ListTaskByAgent(FLAGS_agent_addr, NULL);
    return 0;
}

int KillTask(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    galaxy->KillTask(FLAGS_task_id);
    return 0;
}

int KillJob(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    galaxy->TerminateJob(FLAGS_job_id);
    return 0;
}

int UpdateJob(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    galaxy::JobDescription job;
    job.replicate_count = FLAGS_replicate_num;
    job.job_id  =  FLAGS_job_id;
    galaxy->UpdateJob(job);
    return 0;
}

int TagAgent(){
    galaxy::Galaxy* galaxy = galaxy::Galaxy::ConnectGalaxy(FLAGS_master_addr);
    if (FLAGS_tag.empty()) {
        fprintf(stderr, "--tag  which can not be empty is required ");
        return -1;
    }
    if (FLAGS_agent_list.empty()) {
        fprintf(stderr, "--tag  which can not be empty is required ");
        return -1;
    }
    std::set<std::string> agents;
    boost::split(agents, FLAGS_agent_list, boost::is_any_of(","));
    if (agents.size() <= 0) {
        fprintf(stderr, "--agent_list  is invalid ");
        return -1;
    }
    bool ret = galaxy->TagAgent(FLAGS_tag, &agents);
    if (!ret) {
        fprintf(stderr, "fail to tag agent ");
    }
    return ret ? 0:-1;
}

int main(int argc, char* argv[]) {

    ::google::SetUsageMessage(USAGE);
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    if(argc < 2){
        fprintf(stderr, "subcommand is required , eg ./galaxy_client list --task_id=9527");
        return -1;
    }
    if(FLAGS_master_addr.empty()){
        fprintf(stderr, "--master_addr which can not be empty  is required ");
        return -1;
    }
    if (strcmp(argv[1], "add") == 0) {
        return ProcessNewJob();
    } else if (strcmp(argv[1], "list") == 0) {
        return ListTask();
    } else if (strcmp(argv[1], "listjob") == 0) {
        return ListJob();
    } else if (strcmp(argv[1], "listnode") == 0) {
        return ListNode();
    } else if (strcmp(argv[1], "kill") == 0) {
        return KillTask();
    } else if (strcmp(argv[1], "killjob") == 0) {
        return KillJob();
    } else if (strcmp(argv[1], "listtaskbyagent") == 0){
        return ListTaskByAgent();
    } else if (strcmp(argv[1], "updatejob") == 0) {
        return UpdateJob();
    
    } else if (strcmp(argv[1], "tagagent") == 0) {
        return TagAgent();
    }
    else {
        fprintf(stderr,"use ./galaxy_client --help for help");
        return -1;
    }

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
