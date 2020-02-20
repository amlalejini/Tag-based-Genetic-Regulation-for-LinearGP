library(ggplot2)  # (Wickham, 2009)
library(dplyr)    # (Wickham et al., 2018)
library(cowplot)  # (Wilke, 2018)
library(viridis)  # (Garnier, 2018)
library(scales)

data_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/data/reg-graph-comps/alt-sig/"
dump_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/analysis/"
trace_img_dump_dir <- paste(dump_dir, "imgs/reg-network-comps/alt-sig-traces/", sep="")
trace_ids <- 301:400
generation <- 1000


max_fit_org_data_loc <- paste(data_dir, "max_fit_orgs.csv", sep="")
max_fit_org_data <- read.csv(max_fit_org_data_loc, na.strings="NONE")
max_fit_org_data <- subset(max_fit_org_data, select = -c(program))

# Define function to summarize regulation/memory configurations.
get_con <- function(reg, mem) {
  if (reg == "0" && mem == "0") {
    return("none")
  } else if (reg == "0" && mem=="1") {
    return("memory")
  } else if (reg=="1" && mem=="0") {
    return("regulation")
  } else if (reg=="1" && mem=="1") {
    return("both")
  } else {
    return("UNKNOWN")
  }
}
# Specify experimental condition for each datum.
max_fit_org_data$condition <- mapply(get_con, 
                                     max_fit_org_data$USE_FUNC_REGULATION, 
                                     max_fit_org_data$USE_GLOBAL_MEMORY)
max_fit_org_data$condition <- factor(max_fit_org_data$condition,
                                     levels=c("regulation", "memory", "none", "both"))
# trace_id <- 386
for (trace_id in trace_ids) {

  org_info <- filter(max_fit_org_data, SEED==trace_id)
  num_envs <- org_info$NUM_SIGNAL_RESPONSES
  score <- org_info$score
  condition <- org_info$condition
  is_sol <- org_info$solution
  if (is_sol == "0") {
    next
  }
  
  trace_file <- paste(data_dir, 
                      "traces/trace_update-", generation,
                      "_run-id-", trace_id, ".csv", sep="")
  trace_data <- read.csv(trace_file, na.strings="NONE")
  # Do some trace data clean up/add extra fields.
  trace_data$similarity_score <- 1 - trace_data$match_score
  trace_data$triggered <- (trace_data$env_signal_closest_match == trace_data$module_id) & (trace_data$cpu_step == "0")
  trace_data$is_running <- trace_data$is_running > 0 | trace_data$triggered | trace_data$is_cur_responding_function == "1"

  # correct response ids  
  response_time_steps <- levels(factor(filter(trace_data, is_cur_responding_function=="1")$time_step))
  responses_by_env_update <- list()
  for (t in response_time_steps) {
    env_update <- levels(factor(filter(trace_data, time_step==t)$env_cycle))
    if (env_update %in% responses_by_env_update) {
      if (as.integer(t) > as.integer(responses_by_env_update[env_update])) {
        responses_by_env_update[env_update] = t
      } 
    } else {
      responses_by_env_update[env_update] = t
    }
  }
  # for (env_update in responses_by_env_update) {
  #   print(env_update)
  # }
  
  # Build a list of modules that were triggered
  triggered_ids <- levels(factor(filter(trace_data, triggered==TRUE)$module_id))
  response_ids <- levels(factor(filter(trace_data, is_cur_responding_function=="1")$module_id))
  
  trace_data$is_ever_active <- trace_data$is_ever_active=="1" | 
    trace_data$is_running | 
    trace_data$module_id %in% triggered_ids | 
    trace_data$module_id %in% response_ids
  
  trace_data$is_cur_responding_function <- trace_data$is_cur_responding_function=="1" &
                                           trace_data$time_step %in% responses_by_env_update
  
  categorize_reg_state <- function(reg_state) {
    if (reg_state == 0) {
      return("neutral")
    } else if (reg_state < 0) {
      return("promoted")
    } else if (reg_state > 0) {
      return("repressed")
    } else {
      return("unknown")
    }
  }
  trace_data$regulator_state_simplified <- mapply(categorize_reg_state,
                                                  trace_data$regulator_state)
  
  
  # Extract only rows that correspond with modules that were active during evaluation.
  active_data <- filter(trace_data, is_ever_active==TRUE)
  # Do some work to have module ids appear in a nice order along axis.
  active_module_ids <- levels(factor(active_data$module_id))
  active_module_ids <- as.integer(active_module_ids)
  module_id_map <- as.data.frame(active_module_ids)
  module_id_map$order <- order(module_id_map$active_module_ids) - 1
  get_module_x_pos <- function(module_id) {
    return(filter(module_id_map, active_module_ids==module_id)$order)
  }
  active_data$mod_id_x_pos <- mapply(get_module_x_pos, active_data$module_id)
  trace_data <- active_data # omit all non-active modules from visualization
  
  title <- paste("Condition: ",condition, "; # Envs: ", num_envs, ";\nScore: ", score, "; Solution? ", is_sol , sep="")
  out_name <- paste(trace_img_dump_dir, "trace-id-", trace_id, "-regulator-state.pdf", sep="")
  ggplot(trace_data, 
         aes(x=mod_id_x_pos, 
             y=time_step, 
             fill=regulator_state_simplified)) +
    scale_fill_viridis(name="State:",
                       limits=c("promoted", "neutral", "repressed"),
                       labels=c("Promoted", "Neutral", "Repressed"),
                       discrete=TRUE, 
                       direction=-1) +
    scale_x_discrete(name="Function ID",
                     limits=seq(0, length(active_module_ids)-1, 1),
                     labels=active_module_ids) +
    ylab("Time Step") +
    # Background
    geom_tile(color="white", 
              size=0.2, 
              width=1, 
              height=1) +
    # Module is-running highlights
    geom_tile(data=filter(trace_data, is_running==TRUE), 
              color="black", 
              width=1, 
              height=1, 
              size=0.8) +
    # Environment delimiters
    geom_hline(yintercept=filter(trace_data, cpu_step==0)$time_step - 0.5, 
               size=1) +
    # Draw points on triggered modules
    geom_point(data=filter(trace_data, triggered==TRUE),
               shape=21, colour="black", fill="white", stroke=0.33, size=0.75,
               position=position_nudge(x = 0, y = 0.01)) +
    geom_point(data=filter(trace_data, is_cur_responding_function==TRUE),
               shape=21, colour="white", fill="tomato", stroke=0.33, size=0.75,
               position=position_nudge(x = 0, y = 0.01)) +
    theme(legend.position = "top",
          legend.text = element_text(size=10),
          axis.text.y = element_text(size=10),
          axis.text.x = element_text(size=10)) +
    ggtitle(title) +
    ggsave(out_name, height=7, width=5)
  
  out_name <- paste(trace_img_dump_dir, "trace-id-", trace_id, "-similarity-score.pdf", sep="")
  ggplot(trace_data, 
         aes(x=mod_id_x_pos, 
             y=time_step, 
             fill=similarity_score)) +
    scale_fill_viridis(option="plasma",
                       name="Score:  ",
                       limits=c(0, 1.0),
                       breaks=c(0, 0.25, 0.50, 0.75, 1.0),
                       labels=c("0%", "25%", "50%", "75%", "100%")) +
    scale_x_discrete(name="Function ID",
                     limits=seq(0, length(active_module_ids)-1, 1),
                     labels=active_module_ids) +
    # Background
    geom_tile(color="white", 
              size=0.2, 
              width=1, 
              height=1) +
    # Module is-running highlights
    geom_tile(data=filter(trace_data, is_running==TRUE | triggered==TRUE), 
              color="black", 
              width=1, 
              height=1, 
              size=0.8) +
    # Environment delimiters
    geom_hline(yintercept=filter(trace_data, cpu_step==0)$time_step-0.5, size=1) +
    # Draw points on triggered modules
    geom_point(data=filter(trace_data, triggered==TRUE),
               shape=21, colour="black", fill="white", stroke=0.33, size=0.75,
               position=position_nudge(x = 0, y = 0.01)) +
    geom_point(data=filter(trace_data, is_cur_responding_function==TRUE),
               shape=21, colour="white", fill="tomato", stroke=0.33, size=0.75,
               position=position_nudge(x = 0, y = 0.01)) +
    theme(legend.position = "top",
          legend.text = element_text(size=10),
          axis.text.y = element_text(size=10),
          axis.text.x = element_text(size=10)) +
    guides(fill = guide_colourbar(barwidth = 10, barheight = 0.5)) +
    ggtitle(title) +
    ggsave(out_name, height=7, width=5)
}

